#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "editor.h"

struct termios original_settings;
struct termios settings;

struct winsize window;

int cursor_row;
int cursor_col;

void editor_init(void) {
    cursor_row = 0;
    cursor_col = 0;

    editor_enable_raw_mode();

    while (1) {
        editor_refresh_screen();
        char key = editor_read_key();

        switch (key) {
            case CTRL_P:
                if (cursor_row > 0)
                    cursor_row--;
                break;

            case CTRL_N:
                if (cursor_row < window.ws_row - 1)
                    cursor_row++;
                break;

            case CTRL_B:
                if (cursor_col > 0)
                    cursor_col--;
                break;

            case CTRL_F:
                if (cursor_col < window.ws_col - 1)
                    cursor_col++;
                break;
        }
    }
}

/* turn off canonical input and echo */
void editor_enable_raw_mode(void) {
    tcgetattr(STDIN_FILENO, &original_settings);
    settings = original_settings;
    settings.c_lflag &= ~(ICANON | ECHO);
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

char editor_read_key(void) {
    char key;
    read(STDIN_FILENO, &key, 1);
    return key;
}

void editor_refresh_screen(void) {
    printf("\x1b[2J");    /* clear the screen */
    printf("\x1b[H");    /* move to the top-left */
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &window);    /* get terminal size */
    editor_draw_rows();
    printf("\x1b[%d;%dH", cursor_row + 1, cursor_col + 1);

    fflush(stdout);
}

void editor_clear_screen(void) {
    
}

void editor_draw_rows(void) {
    for (int i = 0; i < window.ws_row; i++) {
        printf("~");

        if (i < window.ws_row - 1)
            printf("\r\n");
    }
}
