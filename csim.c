// 2009-11744 심규민

/**
 * 구현 개요
 *
 * Options
 *  옵션은 getopt를 이용하여 받는다.
 *  구현된 옵션은 s, E, b, t 이다.
 *
 * Cache
 *  cache는 여러 set으로 구성되어 있다.
 *  set들의 pointer의 array로 구현되어 있다.
 *
 * Set
 *  set은 여러 line으로 구성되어 있다.
 *  line의 doubly linked list로 구현되어 있다.
 *
 * Line
 *  line은 tag를 하나 가지고 있다.
 *
 * Access
 *  memory access 요청은 Load, Store, Modify 세가지로
 *  구분해서 들어오는데, Load와 Store는 access 한 번,
 *  Modify는 같은 위치 access 두 번으로 간주한다.
 *
 * Algorithm(LRU)
 *  어떤 access가 들어오면 해당 address로부터
 *  set index와 tag를 구하고,
 *  해당 set의 list에서 tag와 일치하는 line이 있는지 찾는다.
 *  없으면, misses를 증가시키고, 그 tag를 갖는 line을 list 맨뒤에 넣는다.
 *  있으면, hits를 증가시키고, 그 line을 list에서 빼서 맨뒤로 옮긴다.
 *  list에 line을 넣을 때, list의 길이가 E를 초과하면,
 *  evictions를 증가시키고, list의 맨앞에서 line 하나를 뺀다.
 */

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "cachelab.h"

// cache 내 set의 최대 개수
#define MAX_SET (1 << 8)

////////////////////
// Set, Line 구현 //
////////////////////

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

// 주어진 tag를 갖는 새로운 line을 만든다.
struct line_t *new_line(unsigned long long tag) {
    struct line_t *line = (struct line_t *)malloc(sizeof(struct line_t));
    line->tag = tag;
    line->next = NULL;
    line->prev = NULL;
    return line;
}

// 주어진 line을 지운다.
void delete_line(struct line_t *line) {
    free(line);
}

// 주어진 set의 모든 line을 지운다.
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

// 주어진 set의 맨뒤에 주어진 line을 넣는다.
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

// 주어진 set의 맨앞에서 line을 하나 뺀다.
// 빠진 line을 리턴한다.
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

// 주어진 set에서 주어진 tag를 가진 line을 찾는다.
// 없으면 NULL을 리턴한다.
struct line_t *find(struct set_t *set, unsigned long long tag) {
    struct line_t *line = set->head;
    while(line != NULL) {
        if(line->tag == tag) return line;
        line = line->next;
    }
    return NULL;
}

// 주어진 set에서 주어진 line을 뺀다.
// 주어진 line을 그대로 리턴한다.
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

/////////////////
// 전역 변수들 //
/////////////////

// 옵션
static int s, E, b;
static char *t;

// 캐시
static struct set_t cache[MAX_SET];
// 결과
static int hits, misses, evictions;

/////////////////////
// Simulation 구현 //
/////////////////////

// 옵션을 파싱해서 저장한다.
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

// address에서 tag를 구한다.
unsigned long long extractTag(unsigned long long address) {
    return address >> b >> s;
}

// address에서 set index를 구한다.
int extractSetIndex(unsigned long long address) {
    int mask = (1 << s) - 1;
    int block = (int)(address >> b);
    return block & mask;
}

// 주어진 address에 대한 메모리 access를 simulate 한다.
void simulateAccess(unsigned long long address) {
    unsigned long long tag = extractTag(address); // tag를 얻고,
    int si = extractSetIndex(address);            // set index를 얻어서,
    struct set_t *set = &cache[si];               // 해당 set을 찾고,
    struct line_t *line = find(set, tag);         // set에서 tag를 가진 line을 찾는다.
    if(line == NULL) {                   // 없으면 miss
        misses++;
        push_back(set, new_line(tag));
        if(set->count > E) {             // E를 초과하면 eviction
            evictions++;
            delete_line(pop_front(set));
        }
    }else {                              // 있으면 hit
        hits++;
        purge(set, line);
        push_back(set, line);
    }
}

// 주어진 한 줄의 trace를 simulate 한다.
// address를 파싱하여 얻고, Load/Store/Modify에 따라 access 한다.
void simulateOneStep(const char *trace) {
    if(trace[0] == 'I') return; // Instruction은 무시

    unsigned long long address = strtoull(trace + 2, NULL, 16);
    // M은 두 번, L과 S는 한 번
    switch(trace[1]) {
    case 'M':
        simulateAccess(address);
        //break; // 의도 됨
    case 'L':
    case 'S':
        simulateAccess(address);
        break;
    }
}

// cache에 할당했던 메모리들을 해제한다.
void clearCache() {
    for(int i = 0; i < (1 << s); i++) {
        clear(&cache[i]);
    }
}

// 주어진 옵션에 따라 simulate 한다.
// 반드시 parseArgs가 먼저 수행되어야 한다.
// trace 파일을 한 줄씩 읽어서 simulate 한다.
void simulateCache() {
    char buf[32]; // 한 줄의 길이는 32 보다 작다고 가정한다.
    FILE *f = fopen(t, "r");
    while(fgets(buf, sizeof(buf), f) != NULL)
        simulateOneStep(buf);
    fclose(f);
    clearCache();
}

// 옵션을 파싱하고, simulate 해서, 결과를 출력한다.
int main(int argc, char *argv[]) {
    parseArgs(argc, argv);
    simulateCache();
    printSummary(hits, misses, evictions);
    return 0;
}
