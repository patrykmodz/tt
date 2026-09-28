#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>

#include "editor.h"

struct termios original_settings;
struct termios settings;

struct winsize window;

void editor_row_init(struct editor_row *row);
void editor_delete_row(int at);
void editor_row_delete_char(struct editor_row *row, int at);
void editor_handle_resize(int signal);

int cursor_row;
int cursor_col;
int preferred_col;
struct editor_row *rows;
size_t num_rows;
int row_offset;

int command_pending;

volatile sig_atomic_t resized = 0;

void editor_run(void) {
    while(1) {
        editor_refresh_screen();

        if(resized) {
            resized = 0;
            continue;
        }

        char key;
        int result = editor_read_key(&key);

        if(resized) {
            resized = 0;
            continue;
        }

        if(result == -1)
            continue;

        switch(key) {
            case CTRL_P:
                if(cursor_row > 0) {
                    cursor_row--;

                    if(preferred_col > rows[cursor_row].size) {
                        cursor_col = rows[cursor_row].size;
                    } else {
                        cursor_col = preferred_col;
                    }
                }
                break;
            case CTRL_N:
                if(cursor_row < num_rows - 1) {
                    cursor_row++;

                    if(preferred_col > rows[cursor_row].size) {
                        cursor_col = rows[cursor_row].size;
                    } else {
                        cursor_col = preferred_col;
                    }
                }
                break;
            case CTRL_B:
                if(cursor_col > 0) {
                    cursor_col--;
                    preferred_col = cursor_col;
                }
                break;
            case CTRL_F:
                if(cursor_col < rows[cursor_row].size) {
                    cursor_col++;
                    preferred_col = cursor_col;
                }
                break;
            case '\r': /* enter */
            case '\n':
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
                break;
            case '\t':
                for(int i = 0; i < 4; i++) {
                    editor_row_insert_char(&rows[cursor_row], cursor_col, ' ');
                    cursor_col++;
                }

                preferred_col = cursor_col;
                break;
            case 127:  /* backspace */
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
                break;
            case CTRL_X:
                command_pending = 1;
                break;
            case CTRL_C:
                if(command_pending) {
                    editor_disable_raw_mode();
                    editor_clear_screen();
                    exit(0);
                }
                break;
            case CTRL_G:
                command_pending = 0;
                break;
        }
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

    editor_enable_raw_mode();
}

/* turn off canonical input and echo */
void editor_enable_raw_mode(void) {
    tcgetattr(STDIN_FILENO, &original_settings);
    settings = original_settings;
    settings.c_lflag &= ~(ICANON | ECHO | ISIG);
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

    if(cursor_row < row_offset)
        row_offset = cursor_row;

    if(cursor_row >= row_offset + window.ws_row - 1)
        row_offset = cursor_row - (window.ws_row - 1) + 1;

    editor_draw_rows();

    printf("\x1b[%d;%dH",
        cursor_row - row_offset + 1,
        cursor_col + 1
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

        if (file_row < num_rows)
            printf("%s", rows[file_row].chars);
        else
            printf("~");

        if (i < window.ws_row - 1)
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
