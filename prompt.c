#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>

#include "prompt.h"
#define CTRL_G 7 /* avoid adding editor.h here */

void buffer_add_char(char key);
void buffer_init(void);
void get_terminal_dimension(void);
void prompt_draw(const char *message);
char *prompt_parse(char *input);

static char *prompt_buffer;
static size_t prompt_size;
static size_t prompt_capacity;
static struct winsize window;

char *prompt_run(const char *message) {
    get_terminal_dimension();
    buffer_init();
    prompt_draw(message);
    char key;
    while(1) {
        read(STDIN_FILENO, &key, 1);

        /* check if cancelled */
        if(key == CTRL_G) {
            free(prompt_buffer);
            prompt_buffer = NULL;
            return NULL;
        }

        /* read input */
        if(key >= 32 && key <= 126) {
            if(prompt_size < prompt_capacity - 1) {
                if(prompt_size + strlen(message) < window.ws_col) {
                    buffer_add_char(key);
                    prompt_draw(message);
                }
            }
        }
        /* read enter */
        if(key == '\r' || key == '\n') {
            break;
        }
        /* backspace */
        if(key == 127) {
            if(prompt_size > 0) {
                prompt_size--;
                prompt_buffer[prompt_size] = '\0';
                prompt_draw(message);
            }
        }
    }
    return prompt_buffer;
}

void buffer_init() {
    prompt_size = 0;
    prompt_capacity = 4096;
    prompt_buffer = malloc(prompt_capacity);

    if(prompt_buffer == NULL) {
        perror("malloc");
        exit(1);
    }

    prompt_buffer[0] = '\0';
}

void buffer_add_char(char key) {
    prompt_buffer[prompt_size] = key;
    prompt_size++;
    prompt_buffer[prompt_size] = '\0';
}

void get_terminal_dimension() {
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &window); /* gets last row */
}

void prompt_draw(const char *message) {
    printf("\x1b[%d;1H", window.ws_row); /* use terminal row we retrieved */
    printf("\x1b[2K"); /* clear row */
    printf("%s%s", message, prompt_buffer); /* print prompt */
    fflush(stdout);
}

char *prompt(const char *message) { /* middleman */
    char *input = prompt_run(message);
    return prompt_parse(input);
}

char *prompt_parse(char *input) {
    if(input == NULL || input[0] == '\0') {
        return NULL;
    }

    return input;
}
