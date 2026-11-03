#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "statusbar.h"
#include "editor.h"
#include "file.h"

static struct winsize window;

void statusbar_draw(void) {
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &window);

    printf("\x1b[%d;1H", window.ws_row - 1);
    printf("\x1b[47m\x1b[30m"); /* white background black text */
    printf("\x1b[2K"); /* clears the entire current line using the current background colour */

    printf(" tt ");
    printf("  "); /* space inbetween */
    /* display filename */
    if(!current_filename) {
        printf("[newfile]");
    } else {
        printf("%s", current_filename);
    }
    /* modified */
    if(editor_is_modified()) {
        printf(" [+] ");
    } else {
        printf(" "); /* just print a space */
    }
    /* line-column */
    printf("(");
    printf("%d", cursor_row+1);
    printf(",");
    printf("%d", cursor_col+1);
    printf(")");

    printf("\x1b[K"); /* clears from the current cursor position to the end of the line
                         filling the remaining space with the white background. */
    printf("\x1b[0m"); /* resets terminal text formatting: stops the white
                         background and black text from affecting anything drawn afterwards */
}
