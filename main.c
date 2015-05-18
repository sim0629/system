#include <assert.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include "filter.h"

#define NULL ((void *)0)
#define MAX_N 8
#define MAX_CNT 10000000

static char type;
static unsigned int n;

static pthread_t threads[MAX_N];

static int cnt;
static sem_t sem;
static filter_t filter;

// ./run [sem|filter] [2|4|8]
void parse_arguments(int argc, char *argv[]) {
    assert(argc >= 3);
    assert(argv[1][0] == 's' || argv[1][0] == 'f');
    type = argv[1][0];
    assert(argv[2][0] == '2' || argv[2][0] == '4' || argv[2][0] == '8');
    n = (unsigned int)(argv[2][0] - '0');
}

void *count(void *param) {
    unsigned int i = (unsigned int)(unsigned long long)param, c;
    if(type == 's') sem_wait(&sem);
    else filter_lock(&filter, i);
    for(c = 0; c < MAX_CNT; c++)
        cnt++;
    if(type == 's') sem_post(&sem);
    else filter_unlock(&filter, i);
    return NULL;
}

void start_threads() {
    unsigned int i;
    if(type == 's') sem_init(&sem, 0, 1);
    else filter_init(&filter, n);
    for(i = 0; i < n; i++)
        pthread_create(&threads[i], NULL, count, (void *)(unsigned long long)i);
}

void join_threads() {
    unsigned int i;
    for(i = 0; i < n; i++)
        pthread_join(threads[i], NULL);
}

int main(int argc, char *argv[]) {
    parse_arguments(argc, argv);
    start_threads();
    join_threads();
    assert(cnt == MAX_CNT * n);
    printf("cnt = %d\n", cnt);
    return 0;
}
