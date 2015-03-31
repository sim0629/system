// 2009-11744 심규민

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "cachelab.h"

static int s, E, b;
static char *t;

void parseArgs(int argc, char *argv[]) {
    int ch;
    while ((ch = getopt(argc, argv, "s:E:b:t:")) != -1) {
        switch(ch) {
        case 's':
            s = atoi(optarg);
            break;
        case 'E':
            E = atoi(optarg);
            break;
        case 'b':
            b = atoi(optarg);
            break;
        case 't':
            t = optarg;
            break;
        }
    }
}

int main(int argc, char *argv[]) {
    parseArgs(argc, argv);
    printSummary(0, 0, 0);
    return 0;
}
