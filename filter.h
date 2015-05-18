#ifndef SGM_FILTER_H
#define SGM_FILTER_H

#define FILTER_MAX_N 8

typedef struct {
    int n;
    int level[FILTER_MAX_N];
    int victim[FILTER_MAX_N];
} filter_t;

void filter_init(filter_t *filter, unsigned int n);
void filter_lock(filter_t *filter, unsigned int i);
void filter_unlock(filter_t *filter, unsigned int i);

#endif
