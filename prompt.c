#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <time.h>

#include "prompt.h"
#define CTRL_G 7 /* avoid adding editor.h here */

static void buffer_add_char(char key);
static void buffer_init(void);
static void get_terminal_dimension(void);
static char *prompt_parse(char *input);
static void prompt_draw(const char *message);
static char *prompt_run(const char *message);

static char *prompt_message_text; /* owns a copy of the message */
static struct timespec prompt_message_expiry; /* stores its expiration time */
static int prompt_message_active; /* tells us whether there's a message to display */

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

void prompt_message(const char *message, int seconds) {
    /* reject invalid arguments */
    if(!message || seconds < 0) {
        return;
    }
    /* get the current monotonic time */
    struct timespec expiry;
    if(clock_gettime(CLOCK_MONOTONIC, &expiry) == -1) {
        return;
    }
    /* allocate a copy of the message */
    char *new_message = strdup(message);
    if(!new_message) {
        return;
    }
    /* calculate when the message expires */
    expiry.tv_sec += seconds;
    /* replace the previous message */
    free(prompt_message_text);
    prompt_message_text = new_message;
    /* store expiry time and activate the message */
    prompt_message_expiry = expiry;
    prompt_message_active = 1;
}

void prompt_refresh(void) {
    /* nothing to display */
    if(!prompt_message_active)
        return;

    /* get the current monotonic time */
    struct timespec now;

    if(clock_gettime(CLOCK_MONOTONIC, &now) == -1)
        return;

    /* check whether the message has expired */
    if(now.tv_sec > prompt_message_expiry.tv_sec ||
      (now.tv_sec == prompt_message_expiry.tv_sec &&
       now.tv_nsec >= prompt_message_expiry.tv_nsec)) {

        free(prompt_message_text);
        prompt_message_text = NULL;
        prompt_message_active = 0;

        /* clear the prompt row */
        get_terminal_dimension();
        printf("\x1b[%d;1H", window.ws_row);
        printf("\x1b[2K");
        return;
    }

    /* draw the message on the bottom row */
    get_terminal_dimension();
    printf("\x1b[%d;1H", window.ws_row);
    printf("\x1b[2K");
    printf("%s", prompt_message_text);
}
