#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "editor.h"
#include "file.h"

const char *current_filename = NULL; /* gets the compiler to shut the fuck up */

int open_file(const char *filename) {
    FILE *file = fopen(filename, "r");

    if(file==NULL) {
        perror(filename);
        return -1;
    }

    current_filename = filename;
    editor_delete_row(0); /* eliminates random extra row on open */

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

    /* if file empty - give empty line back */
    if(num_rows == 0) {
        num_rows = 1;
        rows = malloc(sizeof(struct editor_row));
        editor_row_init(&rows[0]);
    }

    /* reset cursor positions */
    cursor_row = 0;
    cursor_col = 0;
    editor_update_saved_state();
    fclose(file);
}

int save_file(const char *filename) {
    FILE *file = fopen(filename, "w");

    if(file == NULL) {
        perror(filename);
        return -1;
    }

    for(int i = 0; i < num_rows; i++) {
        fwrite(rows[i].chars, 1, rows[i].size, file);
        fputc('\n', file);
    }

    fclose(file);
    return 0;
}
