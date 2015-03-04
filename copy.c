#include <stdio.h>
#include <sys/time.h>

#define TRY 5
#define MAX_LEN (4096 * 4096)

int a[MAX_LEN], b[MAX_LEN];

int copy(int step) {
    struct timeval before, after;
    int offset, pos;
    gettimeofday(&before, NULL);
    for(offset = 0; offset < step; offset++) {
        for(pos = offset; pos < MAX_LEN; pos += step) {
            a[pos] = b[pos];
        }
    }
    gettimeofday(&after, NULL);
    return (after.tv_sec - before.tv_sec) * 1000000 + (after.tv_usec - before.tv_usec);
}

int main(void) {
    int step, try;
    int elapsed_us;
    copy(1);
    for(step = 1; step < MAX_LEN; step <<= 1) {
        elapsed_us = 0;
        for(try = 0; try < TRY; try++) {
            elapsed_us += copy(step);
        }
        elapsed_us /= TRY;
        printf("%d\t%d\n", step, elapsed_us);
    }
    return 0;
}
