#include <stdio.h>
#include <string.h>
#include "editor.h"


void usage();

int main(int argc, char* argv[]) {

    if(argc==2) {
        if(strcmp(argv[1], "--help")==0) {
            usage();
        }
    } else if(argc==1) {
        editor_init();
    }

    editor_disable_raw_mode();
    return 0;
}

void usage() {
    puts("USAGE:");
}
