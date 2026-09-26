#include <stdio.h>
#include <string.h>

void usage();

int main(int argc, char* argv[]) {

    if(argc==2) {
        if(strcmp(argv[1], "--help")==0) {
            usage();
        }
    }

    return 0;
}

void usage() {
    puts("USAGE:");
}
