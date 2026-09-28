#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "editor.h"
#include "file.h"

int open_file(const char *filename) {
    FILE *file = fopen(filename, "r");

    if(file==NULL) {
        /* will eventually be buffer */
        perror(filename);
        return -1;
    }

    char buffer[1024];
    int row = 0;

    /* keep reading lines until there aren't any */
    while(fgets(buffer, sizeof(buffer), file) != NULL) {
        editor_insert_row(row);
        buffer[strcspn(buffer, "\n")] = '\0';
        for(int i = 0; buffer[i] != '\0'; i++) {
            editor_row_insert_char(&rows[row], rows[row].size, buffer[i]);
        }
        row++;
    }
    /* reset cursor positions */
    cursor_row = 0;
    cursor_col = 0;
    fclose(file);
}
