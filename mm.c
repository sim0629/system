/* *
 * Free block layout
 * [  0,   1): is_allocated
 * [  1,  26): block_size
 * [ 26,  51): left_child
 * [ 51,  52): left_or_right
 * [ 52,  77): parent
 * [ 77, 102): right_child
 * ...
 * [102, 103): is_allocated
 * [103, 128): block_size
 * */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>

#include "mm.h"
#include "memlib.h"

#ifdef MM_CHECK
static int mm_check(void);
#endif

student_t student = {
    "심규민",
    "2009-11744"
};

static void *freeblock_base;
static void *freeblock_root;

static void insert_freeblock(void *block_ptr);
static void remove_freeblock(void *block_ptr);
static void *find_freeblock(size_t block_size);
static void *coalesce_freeblock(void *block_ptr);

#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))

#define BRK_OFFSET 4
#define SBRK_ERROR ((void *)-1)

#define USER_TO_KERNEL(P) ((void *)((char *)(P) - 4))
#define KERNEL_TO_USER(P) ((void *)((char *)(P) + 4))

#define BOUNDARY_SIZE 8
#define ALIGNED_SIZE(SZ) ((((SZ) + 7) & ~7) + BOUNDARY_SIZE)
#define MIN_BLOCK_SIZE 16

#define UNSIGNED(P, I) \
    (*((unsigned *)(P) + (I)))
#define ONES(N) \
    ((1u << (N)) - 1)
#define MASK(U, S, N) \
    (((U) & (ONES(N) << (32 - (S) - (N)))) >> (32 - (S) - (N)))
#define MARK(U, S, N, V) \
    ((U) &= ~(ONES(N) << (32 - (S) - (N))), (U) |= ((V) << (32 - (S) - (N))))
#define CONCAT(UH, UL, N) \
    (((UH) << (N)) | (UL))
#define BASE_PLUS(O) \
    ((void *)((char *)freeblock_base + (O)))
#define MINUS_BASE(P) \
    ((unsigned)((char *)(P) - (char *)freeblock_base))

#define NIL BASE_PLUS(ONES(25))

#define FREED 0
#define ALLOCATED 1

#define LEFT 0
#define RIGHT 1
#define LR_WHATEVER 0

#define IS_ALLOCATED(P) \
    ((int)MASK(UNSIGNED((P), 0), 0, 1))
#define BLOCK_SIZE(P) \
    ((size_t)MASK(UNSIGNED((P), 0), 1, 25))
#define LEFT_CHILD(P) \
    BASE_PLUS(CONCAT(MASK(UNSIGNED((P), 0), 26, 6), \
                     MASK(UNSIGNED((P), 1), 0, 19), 19))
#define LEFT_OR_RIGHT(P) \
    ((int)MASK(UNSIGNED((P), 1), 19, 1))
#define PARENT(P) \
    BASE_PLUS(CONCAT(MASK(UNSIGNED((P), 1), 20, 12), \
                     MASK(UNSIGNED((P), 2), 0, 13), 13))
#define RIGHT_CHILD(P) \
    BASE_PLUS(CONCAT(MASK(UNSIGNED((P), 2), 13, 19), \
                     MASK(UNSIGNED((P), 3), 0, 6), 6))

#define SHIFT(P, O) \
    ((void *)((char *)(P) + O))
#define PREV_IS_ALLOCATED(P) \
    ((int)MASK(UNSIGNED((P), -1), 6, 1))
#define PREV_BLOCK_SIZE(P) \
    ((size_t)MASK(UNSIGNED((P), -1), 7, 25))
#define PREV_BLOCK(P) \
    SHIFT((P), -PREV_BLOCK_SIZE(P))
#define NEXT_BLOCK(P) \
    SHIFT((P), BLOCK_SIZE(P))
#define NEXT_IS_ALLOCATED(P) \
    IS_ALLOCATED(NEXT_BLOCK(P))
#define NEXT_BLOCK_SIZE(P) \
    BLOCK_SIZE(NEXT_BLOCK(P))

#define SET_IS_ALLOCATED(P, A) \
    MARK(UNSIGNED((P), 0), 0, 1, (A)), \
    MARK(UNSIGNED((P), (BLOCK_SIZE(P) >> 2) - 1), 6, 1, (A))
#define SET_BLOCK_SIZE(P, SZ) \
    MARK(UNSIGNED((P), 0), 1, 25, (SZ)), \
    MARK(UNSIGNED((P), ((SZ) >> 2) - 1), 7, 25, (SZ))
#define SET_LEFT_CHILD(P, Q) \
    MARK(UNSIGNED((P), 0), 26, 6, MASK(MINUS_BASE(Q), 7, 6)), \
    MARK(UNSIGNED((P), 1), 0, 19, MASK(MINUS_BASE(Q), 13, 19))
#define SET_LEFT_OR_RIGHT(P, LR) \
    MARK(UNSIGNED((P), 1), 19, 1, (LR))
#define SET_PARENT(P, Q) \
    MARK(UNSIGNED((P), 1), 20, 12, MASK(MINUS_BASE(Q), 7, 12)), \
    MARK(UNSIGNED((P), 2), 0, 13, MASK(MINUS_BASE(Q), 19, 13))
#define SET_RIGHT_CHILD(P, Q) \
    MARK(UNSIGNED((P), 2), 13, 19, MASK(MINUS_BASE(Q), 7, 19)), \
    MARK(UNSIGNED((P), 3), 0, 6, MASK(MINUS_BASE(Q), 26, 6))

static inline void set_link(void *parent, void *child, int lr) {
    if(child != NIL) {
        SET_LEFT_OR_RIGHT(child, lr);
        SET_PARENT(child, parent);
    }
    
    if(parent != NIL) {
        if(lr == LEFT)
            SET_LEFT_CHILD(parent, child);
        else
            SET_RIGHT_CHILD(parent, child);
    }else {
        freeblock_root = child;
    }
}

static int mm_init_()
{
    void *p = mem_sbrk(BRK_OFFSET);

    if(p == SBRK_ERROR)
        return -1;

    freeblock_base = SHIFT(p, BRK_OFFSET);
    freeblock_root = NIL;

    return 0;
}
int mm_init()
{
    int ret = mm_init_();
    if(ret < 0)
        return ret;
#ifdef MM_CHECK
    printf("=== init ===\n");
    assert(mm_check());
#endif
    return ret;
}

static void *mm_malloc_(size_t size)
{
    size_t block_size = ALIGNED_SIZE(size);
    void *block_ptr = find_freeblock(block_size);

    if(block_ptr == NIL) {
        block_ptr = mem_sbrk(block_size);
        if(block_ptr == SBRK_ERROR)
            return NULL;
        SET_BLOCK_SIZE(block_ptr, block_size);
    }else {
        remove_freeblock(block_ptr);
        if(BLOCK_SIZE(block_ptr) >= block_size + MIN_BLOCK_SIZE) {
            void *new_block_ptr = SHIFT(block_ptr, block_size);
            SET_BLOCK_SIZE(new_block_ptr, BLOCK_SIZE(block_ptr) - block_size);
            SET_IS_ALLOCATED(new_block_ptr, FREED);
            insert_freeblock(new_block_ptr);
            SET_BLOCK_SIZE(block_ptr, block_size);
        }
    }

    SET_IS_ALLOCATED(block_ptr, ALLOCATED);
    return KERNEL_TO_USER(block_ptr);
}
void *mm_malloc(size_t size)
{
    void *ret = NULL;
    if(size > 0)
        ret = mm_malloc_(size);
#ifdef MM_CHECK
    printf("=== malloc %u ===\n", size);
    assert(mm_check());
#endif
    return ret;
}

static void mm_free_(void *ptr)
{
    void *block_ptr = USER_TO_KERNEL(ptr);

    SET_IS_ALLOCATED(block_ptr, FREED);
    block_ptr = coalesce_freeblock(block_ptr);
    insert_freeblock(block_ptr);
}
void mm_free(void *ptr)
{
    if(ptr != NULL)
        mm_free_(ptr);
#ifdef MM_CHECK
    printf("=== free %p ===\n", ptr);
    assert(mm_check());
#endif
}

static void *mm_realloc_(void *old_ptr, size_t new_size)
{
    void *new_ptr = mm_malloc_(new_size);
    size_t old_size = BLOCK_SIZE(USER_TO_KERNEL(old_ptr)) - BOUNDARY_SIZE;

    if(new_ptr != NULL)
        memcpy(new_ptr, old_ptr, MIN(old_size, new_size));
    mm_free_(old_ptr);

    return new_ptr;
}
void *mm_realloc(void *ptr, size_t size)
{
    void *ret = NULL;
    if(ptr == NULL && size > 0)
        ret = mm_malloc_(size);
    else if(ptr != NULL && size == 0)
        mm_free_(ptr);
    else if(ptr != NULL && size > 0)
        ret = mm_realloc_(ptr, size);
    else
        ret = NULL;
#ifdef MM_CHECK
    printf("=== realloc %p %u ===\n", ptr, size);
    assert(mm_check());
#endif
    return ret;
}

static void insert_freeblock(void *block_ptr)
{
    size_t block_size = BLOCK_SIZE(block_ptr);
    void *p = freeblock_root;
    int lr = LR_WHATEVER;

    SET_LEFT_CHILD(block_ptr, NIL);
    SET_RIGHT_CHILD(block_ptr, NIL);

    while(p != NIL) {
        void *q;
        if(block_size < BLOCK_SIZE(p)) {
            q = LEFT_CHILD(p);
            if(q == NIL) {
                lr = LEFT;
                break;
            }
        }else {
            q = RIGHT_CHILD(p);
            if(q == NIL) {
                lr = RIGHT;
                break;
            }
        }
        p = q;
    }

    set_link(p, block_ptr, lr);
}

static void remove_freeblock(void *block_ptr)
{
    void *left = LEFT_CHILD(block_ptr);
    void *right = RIGHT_CHILD(block_ptr);
    void *parent = PARENT(block_ptr);
    int lr = LEFT_OR_RIGHT(block_ptr);
    void *target;

    if(left == NIL && right == NIL) {
        target = NIL;
    }else if(left == NIL && right != NIL) {
        target = right;
    }else if(left != NIL && right == NIL) {
        target = left;
    }else {
        void *p = right, *pp = right;
        while(1) {
            void *q = LEFT_CHILD(p);
            if(q == NIL)
                break;
            pp = p;
            p = q;
        }
        if(p == right) {
            set_link(p, left, LEFT);
        }else {
            set_link(pp, RIGHT_CHILD(p), LEFT);
            set_link(p, left, LEFT);
            set_link(p, right, RIGHT);
        }
        target = p;
    }

    set_link(parent, target, lr);
}

static void *find_freeblock(size_t block_size)
{
    void *p = freeblock_root, *fit = NIL;

    while(p != NIL) {
        if(block_size <= BLOCK_SIZE(p))
            fit = p;
        if(block_size < BLOCK_SIZE(p)) {
            p = LEFT_CHILD(p);
        }else {
            p = RIGHT_CHILD(p);
        }
    }

    return fit;
}

static void *coalesce_freeblock(void *block_ptr)
{
    void *cur = block_ptr;

    if(cur > freeblock_base) {
        if(PREV_IS_ALLOCATED(cur) == FREED) {
            void *prev = PREV_BLOCK(cur);
            size_t size = BLOCK_SIZE(prev) + BLOCK_SIZE(cur);
            remove_freeblock(prev);
            SET_BLOCK_SIZE(prev, size);
            cur = prev;
        }
    }

    if(NEXT_BLOCK(cur) <= mem_heap_hi()) {
        if(NEXT_IS_ALLOCATED(cur) == FREED) {
            void *next = NEXT_BLOCK(cur);
            size_t size = BLOCK_SIZE(cur) + BLOCK_SIZE(next);
            remove_freeblock(next);
            SET_BLOCK_SIZE(cur, size);
        }
    }

    return cur;
}

#ifdef MM_CHECK

static size_t traversal(void *block_ptr)
{
    size_t left_cnt, right_cnt;
    if(block_ptr == NIL) return 0;
    left_cnt = traversal(LEFT_CHILD(block_ptr));
    printf("ptr: %p, size: %u\n", block_ptr, BLOCK_SIZE(block_ptr));
    right_cnt = traversal(RIGHT_CHILD(block_ptr));
    return left_cnt + 1 + right_cnt;
}

static int mm_check()
{
    void *block_ptr = freeblock_base;
    size_t freeblock_cnt = 0;
    while(block_ptr <= mem_heap_hi()) {
        size_t block_size = BLOCK_SIZE(block_ptr);
        int is_allocated = IS_ALLOCATED(block_ptr);
        if(is_allocated == FREED) freeblock_cnt++;
        printf("size: %u, alloc: %d\n", block_size, is_allocated);
        block_ptr = SHIFT(block_ptr, block_size);
    }
    return traversal(freeblock_root) == freeblock_cnt;
}

#endif
