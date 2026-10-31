#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>
#include <sys/ioctl.h>

#include "editor.h"
#include "file.h"
#include "prompt.h"

struct termios settings;
struct termios original_settings;

struct winsize window;
volatile sig_atomic_t resized = 0;

static void editor_enable_raw_mode(void);
static int editor_read_key(char *key);
static void editor_refresh_screen(void);
static void editor_clear_screen(void);
static void editor_draw_rows(void);

static void editor_row_delete_char(struct editor_row *row, int at);
static void editor_handle_resize(int signal);
static int editor_is_modified(void);
static void editor_free_saved_state(void);

/* key handling */
static void handle_ctrl_p(void);
static void handle_ctrl_n(void);
static void handle_ctrl_f(void);
static void handle_ctrl_b(void);
static void handle_save(void);
static void handle_enter(void);
static void handle_tab(void);
static void handle_backspace(void);
static int handle_exit(void);
static void pend(void);
static void cancel_pend(void);
static int bullshit(void); /* for the funsies */

int cursor_row;
int cursor_col;
int preferred_col;
int row_offset;
int col_offset;
int command_pending;

struct editor_row *rows;
static struct editor_row *saved_rows;
size_t num_rows;
static size_t saved_num_rows;

void editor_run(void) {
    while(1) {
        editor_refresh_screen();
        char key;
        int result = editor_read_key(&key);

        if(result == -1) {
            continue;
        }

        if(resized) {
            resized = 0;
            continue;
        }

        switch(key) {
            case CTRL_P:
                handle_ctrl_p();
                break;
            case CTRL_N:
                handle_ctrl_n();
                break;
            case CTRL_F:
                handle_ctrl_f();
                break;
            case CTRL_B:
                handle_ctrl_b();
                break;
            case '\r':
            case '\n':
                handle_enter();
                break;
            case '\t':
                handle_tab();
                break;
            case 127:
                handle_backspace();
                break;
            case CTRL_X:
                pend();
                break;
            case CTRL_C:
                if(handle_exit())
                    return;
                break;
            case CTRL_G:
                cancel_pend();
                break;
            case CTRL_S:
                handle_save();
                break;
        }
        /* typing */
        if(key >= 32 && key <= 126) {
            editor_row_insert_char(&rows[cursor_row], cursor_col, key);
            cursor_col++;
        }
    }
}

void editor_init(void) {
    command_pending = 0;
    cursor_row = 0;
    row_offset = 0;
    col_offset = 0;
    cursor_col = 0;
    preferred_col = cursor_col;

    struct sigaction action = {0};
    action.sa_handler = editor_handle_resize;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGWINCH, &action, NULL);

    num_rows = 1;
    rows = malloc(sizeof(struct editor_row) * num_rows);
    editor_row_init(&rows[0]);

    editor_update_saved_state();
    editor_enable_raw_mode();
}

/* turn off canonical input and echo */
void editor_enable_raw_mode(void) {
    tcgetattr(STDIN_FILENO, &original_settings);
    settings = original_settings;
    settings.c_lflag &= ~(ICANON | ECHO | ISIG);
    settings.c_iflag &= ~(IXON | IXOFF);
    /* wait until at least one byte is available, then return immediately */
    settings.c_cc[VMIN] = 1;
    settings.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &settings);
}

/* turn on canonical input and echo */
void editor_disable_raw_mode(void) {
    settings = original_settings;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &settings);
}

int editor_read_key(char *key) {
    return read(STDIN_FILENO, key, 1);
}

void editor_refresh_screen(void) {
    printf("\x1b[2J");    /* clear the screen */
    printf("\x1b[3J");    /* clear terminal scroll-back */
    printf("\x1b[H");     /* move to the top-left */

    ioctl(STDOUT_FILENO, TIOCGWINSZ, &window);    /* get terminal size */

    /* vertical scrolling */
    int v_margin = 8; /* 8 lines */
    int visible_rows = window.ws_row - 1;

    if(cursor_row < row_offset + v_margin)
        row_offset = cursor_row - v_margin;

    if(cursor_row >= row_offset + visible_rows - v_margin)
        row_offset = cursor_row - visible_rows + v_margin + 1;

    if(row_offset < 0)
        row_offset = 0;

    /* horizontal scrolling */
    int h_margin = 20; /* 20 chars */
    int visible_cols = window.ws_col - 1;

    if(cursor_col < col_offset + h_margin)
        col_offset = cursor_col - h_margin;

    if(cursor_col >= col_offset + visible_cols - h_margin)
        col_offset = cursor_col - visible_cols + h_margin + 1;

    if(col_offset < 0)
        col_offset = 0;

    editor_draw_rows();

    printf("\x1b[%d;%dH",
        cursor_row - row_offset + 1,
        cursor_col - col_offset +1
    );

    fflush(stdout);
}

void editor_clear_screen(void) {
    printf("\x1b[2J");    /* clear the screen */
    printf("\x1b[3J");    /* clear terminal scroll-back */
    printf("\x1b[H");     /* move to cursor to top-left */
    fflush(stdout);
}

void editor_draw_rows(void) {
    for(int i = 0; i < window.ws_row; i++) {
        int file_row = i + row_offset;

        if(file_row < num_rows) {
            struct editor_row *row = &rows[file_row];

            if(col_offset < row->size) {
                int visible = row->size - col_offset;

                if(visible > window.ws_col)
                    visible = window.ws_col;

                printf("%.*s", visible, row->chars + col_offset);
            }
        } else {
            printf("~");
        }

        if(i < window.ws_row - 1)
            printf("\r\n");
    }
}

void editor_row_init(struct editor_row *row) {
    row->size = 0;
    row->capacity = 16;
    row->chars = malloc(row->capacity);

    if(row->chars == NULL) {
        /* allocation failed */
        perror("malloc failed");
        exit(1);
    }

    row->chars[0] = '\0';
}

void editor_row_insert_char(struct editor_row *row, int at, char c) {
    /* grow buffer */
    if(row->size + 2 > row->capacity) { /* +2 for the new char + '\0' */
        size_t new_capacity = row->capacity * 2;
        char *new_chars = realloc(row->chars, new_capacity);

        if(new_chars == NULL) {
            /* allocation failed */
            perror("realloc failed");
            exit(1);
        }

        row->chars = new_chars;
        row->capacity = new_capacity;
    }

    /* move everything from row->size backwards */
    for (int i = row->size; i >= at; i--) {
        row->chars[i + 1] = row->chars[i];
    }
    row->chars[at] = c;
    row->size++;
}

void editor_row_delete_char(struct editor_row *row, int at) {
    /* move everything from row->size forwards */
    for (int i = at; i < row->size; i++) {
        row->chars[i] = row->chars[i + 1];
    }
    row->size--;
}

void editor_insert_row(int at) {
    struct editor_row *new_rows =
        realloc(rows, (num_rows + 1) * sizeof(struct editor_row));

    if(new_rows == NULL) {
        /* allocation failed */
        perror("realloc failed");
        exit(1);
    }

    rows = new_rows;

    /* move rows after at one position forwards */
    for(int i = num_rows; i > at; i--) {
        rows[i] = rows[i - 1];
    }

    editor_row_init(&rows[at]);
    num_rows++;
}

void editor_delete_row(int at) {
    /* move everything after at one position backwards */
    for(int i = at; i < num_rows - 1; i++) {
        rows[i] = rows[i + 1];
    }

    num_rows--;

    struct editor_row *new_rows =
        realloc(rows, num_rows * sizeof(struct editor_row));

    if(new_rows == NULL && num_rows > 0) {
        /* allocation failed */
        perror("realloc failed");
        exit(1);
    }

    rows = new_rows;
}

void editor_handle_resize(int signal) {
    (void)signal;
    resized = 1;
}

void editor_free_saved_state(void) {
    for(size_t i = 0; i < saved_num_rows; i++) {
        free(saved_rows[i].chars);
    }

    free(saved_rows);

    saved_rows = NULL;
    saved_num_rows = 0;
}

void editor_update_saved_state(void) {
    editor_free_saved_state();

    saved_num_rows = num_rows;

    if(saved_num_rows == 0) {
        saved_rows = NULL;
        return;
    }

    saved_rows = malloc(sizeof(struct editor_row) * saved_num_rows);

    if(saved_rows == NULL) {
        perror("malloc");
        exit(1);
    }

    for(size_t i = 0; i < saved_num_rows; i++) {
        saved_rows[i].size = rows[i].size;
        saved_rows[i].capacity = rows[i].size + 1;

        saved_rows[i].chars = malloc(saved_rows[i].capacity);

        if(saved_rows[i].chars == NULL) {
            perror("malloc");
            exit(1);
        }

        memcpy(
            saved_rows[i].chars,
            rows[i].chars,
            rows[i].size + 1
        );
    }
}

int editor_is_modified(void) {
    if(num_rows != saved_num_rows) {
        return 1;
    }

    for(size_t i = 0; i < num_rows; i++) {
        if(rows[i].size != saved_rows[i].size) {
            return 1;
        }

        if(strcmp(rows[i].chars, saved_rows[i].chars) != 0) {
            return 1;
        }
    }

    return 0;
}

/* key handling */
void handle_ctrl_p(void) {
    if(cursor_row > 0) {
        cursor_row--;

        if(preferred_col > rows[cursor_row].size) {
            cursor_col = rows[cursor_row].size;
        } else {
            cursor_col = preferred_col;
        }
    }
}

void handle_ctrl_n(void) {
    if(cursor_row < num_rows - 1) {
        cursor_row++;

        if(preferred_col > rows[cursor_row].size) {
            cursor_col = rows[cursor_row].size;
        } else {
            cursor_col = preferred_col;
        }
    }
}

void handle_ctrl_f(void) {
    if(cursor_col < rows[cursor_row].size) {
        cursor_col++;
        preferred_col = cursor_col;
    }
}

void handle_ctrl_b(void) {
    if(cursor_col > 0) {
        cursor_col--;
        preferred_col = cursor_col;
    }
}

void handle_save(void) {
    bullshit();
}

void handle_enter(void) {
    editor_insert_row(cursor_row + 1);

    /* move text after cursor into the new row */
    for(int i = cursor_col; i < rows[cursor_row].size; i++) {
        editor_row_insert_char(
            &rows[cursor_row + 1],
            rows[cursor_row + 1].size,
            rows[cursor_row].chars[i]
        );
    }

    /* terminate the current row at the cursor */
    rows[cursor_row].chars[cursor_col] = '\0';
    rows[cursor_row].size = cursor_col;

    cursor_row++;
    cursor_col = 0;
    preferred_col = 0;
}

void handle_tab(void) {
    int tab_amount = 4;
    for(int i = 0; i < tab_amount; i++) {
        editor_row_insert_char(&rows[cursor_row], cursor_col, ' ');
        cursor_col++;
    }

    preferred_col = cursor_col;
}

void handle_backspace(void) {
    if(cursor_col > 0) {
        cursor_col--;
        preferred_col = cursor_col;
        editor_row_delete_char(&rows[cursor_row], cursor_col);
    } else if(cursor_row > 0) {
        editor_delete_row(cursor_row);
        cursor_row--;
        cursor_col = rows[cursor_row].size;
        preferred_col = cursor_col;
    }
}

static int handle_exit(void) {
    if(!command_pending)
        return 0;

    if(!editor_is_modified()) {
        editor_clear_screen();
        editor_disable_raw_mode();
        return 1;
    }

    char *input = prompt("save changes? (y/n): ");

    if(!input)
        return 0;

    if(strcmp(input, "y") == 0) {
        free(input);

        if(!bullshit())
            return 0;
    } else if(strcmp(input, "n") == 0) {
        free(input);
    } else {
        free(input);
        return 0;
    }

    editor_clear_screen();
    editor_disable_raw_mode();
    return 1;
}

void pend(void) {
    command_pending = 1;
}

void cancel_pend(void) {
    command_pending = 0;
}

/* all bullshit is: saved_as_prompt its also slightly vibecoded hehe*/
int bullshit(void) {
    char cwd[PATH_MAX];
    char *directory;
    char *prompt_text;
    char *filename;
    char *full_path;

    /* determine directory */
    if(current_filename) {
        char *slash = strrchr(current_filename, '/');

        if(slash) {
            size_t len = slash - current_filename;
            directory = malloc(len + 1);
            if(!directory)
                return 0;

            memcpy(directory, current_filename, len);
            directory[len] = '\0';
        } else {
            directory = strdup(".");
        }
    } else {
        if(!getcwd(cwd, sizeof(cwd)))
            return 0;

        directory = strdup(cwd);
    }

    if(!directory)
        return 0;

    /* ask for filename */
    prompt_text = malloc(strlen(directory) + 12);
    if(!prompt_text) {
        free(directory);
        return 0;
    }

    sprintf(prompt_text, "save as: %s/", directory);

    filename = prompt(prompt_text);

    free(prompt_text);

    if(!filename || !filename[0]) {
        free(filename);
        free(directory);
        return 0;
    }

    /* construct full path */
    full_path = malloc(strlen(directory) + strlen(filename) + 2);
    if(!full_path) {
        free(filename);
        free(directory);
        return 0;
    }

    sprintf(full_path, "%s/%s", directory, filename);

    free(filename);
    free(directory);

    /* save */
    if(save_file(full_path) != 0) {
        free(full_path);
        return 0;
    }

    free(current_filename);
    current_filename = strdup(full_path);

    if(!current_filename) {
        free(full_path);
        return 0;
    }
}
