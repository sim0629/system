#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>

#include "mm.h"
#include "memlib.h"

student_t student = {
    "심규민",
    "2009-11744"
};

#define MIN(x, y) ((x) < (y) ? (x) : (y))

#define BRK_OFFSET 4
#define SBRK_ERROR ((void *)-1)

#define USER_TO_KERNEL(ptr) ((void *)((char *)(ptr) - 4))
#define KERNEL_TO_USER(block_ptr) ((void *)((char *)(block_ptr) + 4))
#define BLOCK_SIZE(block_ptr) (*(size_t *)(block_ptr))
#define SET_BLOCK_SIZE(block_ptr, block_size) (BLOCK_SIZE((char *)(block_ptr) + (block_size) - 4) = BLOCK_SIZE(block_ptr) = (block_size))
#define IS_ALLOCATED(block_ptr) (BLOCK_SIZE(block_ptr) & 0x1)
#define MARK_ALLOCATED(block_ptr) (BLOCK_SIZE(block_ptr) |= 0x1)
#define MARK_NOT_ALLOCATED(block_ptr) (BLOCK_SIZE(block_ptr) &= ~0x1)
#define NEXT_BLOCK(cur) (*(void **)((char *)(cur) + 4))
#define PREV_BLOCK(cur) (*(void **)((char *)(cur) + 8))

static void *freeblock_root;

static void insert_freeblock(void *block_ptr);
static void remove_freeblock(void *block_ptr);

static void *alloc_old_freeblock(size_t block_size);
static void *alloc_new_freeblock(size_t block_size);

static int mm_check(void);

int mm_init(void)
{
    void *p = mem_sbrk(BRK_OFFSET);

    if(p == SBRK_ERROR)
        return -1;

    freeblock_root = NULL;

    return 0;
}

static void *mm_malloc_inner(size_t size)
{
    size_t aligned_size = (size + 7) & ~0x7;
    size_t block_size = aligned_size + 8;
    void *block_ptr;

    if(size == 0)
        return NULL;

    block_ptr = alloc_old_freeblock(block_size);
    if(block_ptr == NULL)
        return alloc_new_freeblock(block_size);
    else
        return block_ptr;
}
void *mm_malloc(size_t size)
{
    void *ret = mm_malloc_inner(size);
#ifdef CHECK
    printf("=== malloc %u ===\n", size);
    assert(mm_check());
#endif
    return ret;
}

static void mm_free_inner(void *ptr)
{
    void *block_ptr = USER_TO_KERNEL(ptr);
    MARK_NOT_ALLOCATED(block_ptr);
    insert_freeblock(block_ptr);
}
void mm_free(void *ptr)
{
    mm_free_inner(ptr);
#ifdef CHECK
    printf("=== free %p ===\n", ptr);
    assert(mm_check());
#endif
}

static void *mm_realloc_inner(void *old_ptr, size_t new_size)
{
    void *new_ptr = mm_malloc(new_size);
    size_t old_size = BLOCK_SIZE(USER_TO_KERNEL(old_ptr)) & ~0x1;

    if(new_ptr != NULL)
        memcpy(new_ptr, old_ptr, MIN(old_size, new_size));
    mm_free(old_ptr);

    return new_ptr;
}
void *mm_realloc(void *old_ptr, size_t new_size)
{
    void *ret = mm_realloc_inner(old_ptr, new_size);
#ifdef CHECK
    printf("=== realloc %p %u ===\n", old_ptr, new_size);
    assert(mm_check());
#endif
    return ret;
}

static void insert_freeblock(void *block_ptr)
{
    void *cur = block_ptr;
    size_t cur_size = BLOCK_SIZE(cur);
    void *front, *back;

    front = (void *)((char *)cur - 4);
    if(front >= mem_heap_lo() + BRK_OFFSET) {
        front = (void *)((char *)cur - BLOCK_SIZE(front));
        if(IS_ALLOCATED(front)) {
            front = NULL;
        }
    }else {
        front = NULL;
    }

    back = (void *)((char *)cur + cur_size);
    if(back <= mem_heap_hi()) {
        if(IS_ALLOCATED(back)) {
            back = NULL;
        }
    }else {
        back = NULL;
    }

    if(front == NULL && back == NULL) {
        NEXT_BLOCK(cur) = freeblock_root;
        PREV_BLOCK(cur) = NULL;
        if(freeblock_root != NULL)
            PREV_BLOCK(freeblock_root) = cur;
        freeblock_root = cur;
    }else if(front == NULL && back != NULL) {
        NEXT_BLOCK(cur) = NEXT_BLOCK(back);
        PREV_BLOCK(cur) = PREV_BLOCK(back);
        if(NEXT_BLOCK(cur) != NULL)
            PREV_BLOCK(NEXT_BLOCK(cur)) = cur;
        if(PREV_BLOCK(cur) != NULL)
            NEXT_BLOCK(PREV_BLOCK(cur)) = cur;
        else
            freeblock_root = cur;
        SET_BLOCK_SIZE(cur, cur_size + BLOCK_SIZE(back));
    }else if(front != NULL && back == NULL) {
        SET_BLOCK_SIZE(front, BLOCK_SIZE(front) + cur_size);
    }else if(front != NULL && back != NULL) {
        SET_BLOCK_SIZE(front, BLOCK_SIZE(front) + cur_size + BLOCK_SIZE(back));
        remove_freeblock(back);
    }else {
        assert(0); // unreachable
    }
}

static void remove_freeblock(void *block_ptr)
{
    void *next = NEXT_BLOCK(block_ptr);
    void *prev = PREV_BLOCK(block_ptr);

    if(prev == NULL)
        freeblock_root = next;
    else
        NEXT_BLOCK(prev) = next;

    if(next != NULL)
        PREV_BLOCK(next) = prev;
}

static void *alloc_old_freeblock(size_t block_size)
{
    void *best = NULL, *cur = freeblock_root;
    size_t best_size = UINT_MAX;
    size_t diff_size;

    while(cur != NULL) {
        size_t cur_size = BLOCK_SIZE(cur);
        if(cur_size >= block_size && cur_size < best_size) {
            best_size = cur_size;
            best = cur;
        }
        cur = NEXT_BLOCK(cur);
    }

    if(best == NULL)
        return NULL;

    diff_size = best_size - block_size;

    if(diff_size < 16) {
        remove_freeblock(best);
        MARK_ALLOCATED(best);
        return KERNEL_TO_USER(best);
    }

    SET_BLOCK_SIZE(best, diff_size);
    best = (void *)((char *)best + diff_size);
    SET_BLOCK_SIZE(best, block_size);
    MARK_ALLOCATED(best);
    return KERNEL_TO_USER(best);
}

static void *alloc_new_freeblock(size_t block_size)
{
    void *block_ptr = mem_sbrk(block_size);

    if(block_ptr == SBRK_ERROR)
        return NULL;

    SET_BLOCK_SIZE(block_ptr, block_size);
    MARK_ALLOCATED(block_ptr);
    return KERNEL_TO_USER(block_ptr);
}

static int mm_check()
{
    void *block_ptr = (void *)((char *)mem_heap_lo() + 4);
    while(block_ptr <= mem_heap_hi()) {
        size_t block_size = BLOCK_SIZE(block_ptr) & ~0x1;
        int is_allocated = IS_ALLOCATED(block_ptr);
        printf("size: %u, alloc: %d\n", block_size, is_allocated);
        block_ptr = (void *)((char *)block_ptr + block_size);
    }
    return 1;
}
