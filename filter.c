#include <assert.h>
#include "filter.h"

void filter_init(filter_t *filter, unsigned int n) {
    unsigned int i;
    assert(n <= FILTER_MAX_N);
    filter->n = n;
    for(i = 0; i < n; i++)
        filter->level[i] = 0;
}

void filter_lock(filter_t *filter, unsigned int i) {
    unsigned int l, k;
    for(l = 1; l < filter->n; l++) {
        filter->level[i] = l;
        filter->victim[l] = i;
        // mfence between storing level[i] and loading level[k]
        __sync_synchronize();
        while(filter->victim[l] == i) {
            int exists = 0;
            for(k = 0; k < filter->n; k++) {
                if(k != i && filter->level[k] >= l) {
                    exists = 1;
                    break;
                }
            }
            if(!exists) break;
        }
    }
}

void filter_unlock(filter_t *filter, unsigned int i) {
    filter->level[i] = 0;
}
