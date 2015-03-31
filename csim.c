// 2009-11744 심규민

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "cachelab.h"

#define MAX_SET (1 << 8)

struct line_t {
    unsigned long long tag;
    struct line_t *next;
    struct line_t *prev;
};

struct set_t {
    struct line_t *head;
    struct line_t *tail;
    int count;
};

struct line_t *new_line(unsigned long long tag) {
    struct line_t *line = (struct line_t *)malloc(sizeof(struct line_t));
    line->tag = tag;
    line->next = NULL;
    line->prev = NULL;
    return line;
}

void delete_line(struct line_t *line) {
    free(line);
}

void clear(struct set_t *set) {
    struct line_t *line = set->head, *prev;
    while(line != NULL) {
        prev = line;
        line = line->next;
        free(prev);
    }
    set->head = set->tail = NULL;
    set->count = 0;
}

void push_back(struct set_t *set, struct line_t *line) {
    line->next = NULL;
    line->prev = NULL;
    if(set->count == 0) {
        set->head = set->tail = line;
    }else {
        set->tail->next = line;
        line->prev = set->tail;
        set->tail = line;
    }
    set->count++;
}

struct line_t *pop_front(struct set_t *set) {
    if(set->count == 0) return NULL;
    struct line_t *line = set->head;
    set->head = line->next;
    if(set->head == NULL)
        set->tail = NULL;
    else
        set->head->prev = NULL;
    set->count--;
    return line;
}

struct line_t *find(struct set_t *set, unsigned long long tag) {
    struct line_t *line = set->head;
    while(line != NULL) {
        if(line->tag == tag) return line;
        line = line->next;
    }
    return NULL;
}

struct line_t *purge(struct set_t *set, struct line_t *line) {
    if(line->prev == NULL) return pop_front(set);
    line->prev->next = line->next;
    if(line->next == NULL)
        set->tail = line->prev;
    else
        line->next->prev = line->prev;
    set->count--;
    return line;
}

static int s, E, b;
static char *t;

static struct set_t cache[MAX_SET];
static int hits, misses, evictions;

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

unsigned long long extractTag(unsigned long long address) {
    return address >> b >> s;
}

int extractSetIndex(unsigned long long address) {
    int mask = (1 << s) - 1;
    int block = (int)(address >> b);
    return block & mask;
}

void simulateAccess(unsigned long long address) {
    unsigned long long tag = extractTag(address);
    int si = extractSetIndex(address);
    struct set_t *set = &cache[si];
    struct line_t *line = find(set, tag);
    if(line == NULL) {
        misses++;
        push_back(set, new_line(tag));
        if(set->count > E) {
            evictions++;
            delete_line(pop_front(set));
        }
    }else {
        hits++;
        purge(set, line);
        push_back(set, line);
    }
}

void simulateOneStep(const char *trace) {
    if(trace[0] == 'I') return;

    unsigned long long address = strtoull(trace + 2, NULL, 16);
    switch(trace[1]) {
    case 'M':
        simulateAccess(address);
        //break;
    case 'L':
    case 'S':
        simulateAccess(address);
        break;
    }
}

void clearCache() {
    for(int i = 0; i < (1 << s); i++) {
        clear(&cache[i]);
    }
}

void simulateCache() {
    char buf[32];
    FILE *f = fopen(t, "r");
    while(fgets(buf, sizeof(buf), f) != NULL)
        simulateOneStep(buf);
    fclose(f);
    clearCache();
}

int main(int argc, char *argv[]) {
    parseArgs(argc, argv);
    simulateCache();
    printSummary(hits, misses, evictions);
    return 0;
}
