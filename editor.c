#include <stdio.h>
#include <termios.h>
#include <unistd.h>

#include "editor.h"

struct termios original_settings;
struct termios settings;

void editor_init(void) {
    editor_enable_raw_mode();
    printf("editor initialised\n");
}

/* turn off canonical input and echo */
void editor_enable_raw_mode(void) {
    tcgetattr(STDIN_FILENO, &original_settings);
    settings = original_settings;
    settings.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &settings);
}

/* turn on canonical input and echo */
void editor_disable_raw_mode(void) {
    settings = original_settings;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &settings);
}
