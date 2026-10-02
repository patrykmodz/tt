#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file.h"
#include "editor.h"

void usage();

int main(int argc, char* argv[]) {

    if(argc==2) {
        if(strcmp(argv[1], "--help")==0) {
            usage();
        } else {
            editor_init();
            if(open_file(argv[1]) == -1) {
                editor_disable_raw_mode();
                return 1;
            }
            editor_run();
        }
    } else if(argc==1) {
        editor_init();
        editor_run();
    }

    return 0;
}

void usage() {
    puts("usage: tt [filename]");
}
