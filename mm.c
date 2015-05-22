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
#define IS_ALLOCATED(block_ptr) (BLOCK_SIZE(block_ptr) & 0x1)
#define MARK_ALLOCATED(block_ptr) (BLOCK_SIZE(block_ptr) |= 0x1)
#define MARK_NOT_ALLOCATED(block_ptr) (BLOCK_SIZE(block_ptr) &= ~0x1)
#define LEFT_BLOCK(cur) (*(void **)((char *)(cur) + 4))
#define RIGHT_BLOCK(cur) (*(void **)((char *)(cur) + 8))
typedef enum { LEFT = 4, RIGHT = 8 } child_t;
#define CHILD_BLOCK(cur, child) (*(void **)((char *)(cur) + (int)(child)))

static void *freeblock_root;

static void insert_freeblock(void *block_ptr);
static void remove_freeblock(void *block_ptr);
static void *parent_freeblock(void *block_ptr, child_t *lr_out);

static void *alloc_old_freeblock(size_t block_size);
static void *alloc_new_freeblock(size_t block_size);

static inline void set_block_size(void *block_ptr, size_t block_size) {
    BLOCK_SIZE((char *)block_ptr + block_size - 4) = block_size;
    BLOCK_SIZE(block_ptr) = block_size;
}

static inline void set_child_of_parent(void *block_ptr, void *child) {
    child_t lr;
    void *parent = parent_freeblock(block_ptr, &lr);

    if(parent == NULL)
        freeblock_root = child;
    else
        CHILD_BLOCK(parent, lr) = child;
}

static int mm_check(void);

int mm_init(void)
{
    void *p = mem_sbrk(BRK_OFFSET);

    if(p == SBRK_ERROR)
        return -1;

    freeblock_root = NULL;
    assert(mm_check());

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
    void *cur, *front, *back;

    MARK_NOT_ALLOCATED(block_ptr);
    cur = block_ptr;

    front = (void *)((char *)cur - 4);
    if(front >= mem_heap_lo() + BRK_OFFSET) {
        front = (void *)((char *)cur - BLOCK_SIZE(front));
        if(IS_ALLOCATED(front)) {
            front = NULL;
        }
    }else {
        front = NULL;
    }

    back = (void *)((char *)cur + BLOCK_SIZE(cur));
    if(back <= mem_heap_hi()) {
        if(IS_ALLOCATED(back)) {
            back = NULL;
        }
    }else {
        back = NULL;
    }

    if(front != NULL) {
        remove_freeblock(front);
        set_block_size(front, BLOCK_SIZE(front) + BLOCK_SIZE(cur));
        cur = front;
    }

    if(back != NULL) {
        remove_freeblock(back);
        set_block_size(cur, BLOCK_SIZE(cur) + BLOCK_SIZE(back));
    }

    insert_freeblock(cur);
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
    size_t size = BLOCK_SIZE(block_ptr);
    void *p = freeblock_root;

    LEFT_BLOCK(block_ptr) = NULL;
    RIGHT_BLOCK(block_ptr) = NULL;

    if(p == NULL) {
        freeblock_root = block_ptr;
        return;
    }

    while(1) {
        void *q;
        if(size < BLOCK_SIZE(p)) {
            q = LEFT_BLOCK(p);
            if(q == NULL) {
                LEFT_BLOCK(p) = block_ptr;
                return;
            }
        }else {
            q = RIGHT_BLOCK(p);
            if(q == NULL) {
                RIGHT_BLOCK(p) = block_ptr;
                return;
            }
        }
        p = q;
    }
}

static void remove_freeblock(void *block_ptr)
{
    void *left = LEFT_BLOCK(block_ptr);
    void *right = RIGHT_BLOCK(block_ptr);
    void *target;

    if(left == NULL && right == NULL) {
        target = NULL;
    }else if(left == NULL && right != NULL) {
        target = right;
    }else if(left != NULL && right == NULL) {
        target = left;
    }else {
        void *p = right, *pp = right;
        while(1) {
            void *q = LEFT_BLOCK(p);
            if(q == NULL)
                break;
            pp = p;
            p = q;
        }
        if(p == right) {
            LEFT_BLOCK(p) = left;
        }else {
            LEFT_BLOCK(pp) = RIGHT_BLOCK(p);
            LEFT_BLOCK(p) = left;
            RIGHT_BLOCK(p) = right;
        }
        target = p;
    }
    set_child_of_parent(block_ptr, target);
}

static void *parent_freeblock(void *block_ptr, child_t *lr_out)
{
    size_t size = BLOCK_SIZE(block_ptr);
    void *p = freeblock_root;

    if(block_ptr == freeblock_root)
        return NULL;

    while(p != NULL) {
        void *q;
        if(size < BLOCK_SIZE(p)) {
            q = LEFT_BLOCK(p);
            if(q == block_ptr) {
                if(lr_out != NULL)
                    *lr_out = LEFT;
                break;
            }
        }else {
            q = RIGHT_BLOCK(p);
            if(q == block_ptr) {
                if(lr_out != NULL)
                    *lr_out = RIGHT;
                break;
            }
        }
        p = q;
    }

    return p;
}

static void *alloc_old_freeblock(size_t block_size)
{
    size_t diff_size;
    void *p = freeblock_root, *best = NULL;

    while(p != NULL) {
        void *q;
        if(block_size < BLOCK_SIZE(p)) {
            q = LEFT_BLOCK(p);
            if(q == NULL) {
                best = p;
                break;
            }
        }else if(block_size > BLOCK_SIZE(p)) {
            q = RIGHT_BLOCK(p);
            if(q == NULL) {
                child_t lr;
                q = p;
                while(1) {
                    q = parent_freeblock(q, &lr);
                    if(q == NULL)
                        break;
                    if(lr == LEFT) {
                        best = q;
                        break;
                    }
                }
                break;
            }
        }else {
            best = p;
            break;
        }
        p = q;
    }

    if(best == NULL)
        return NULL;

    remove_freeblock(best);
    diff_size = BLOCK_SIZE(best) - block_size;
    if(diff_size < 16) {
        MARK_ALLOCATED(best);
        return KERNEL_TO_USER(best);
    }

    set_block_size(best, diff_size);
    insert_freeblock(best);

    best = (void *)((char *)best + diff_size);
    set_block_size(best, block_size);
    MARK_ALLOCATED(best);
    return KERNEL_TO_USER(best);
}

static void *alloc_new_freeblock(size_t block_size)
{
    void *block_ptr = mem_sbrk(block_size);

    if(block_ptr == SBRK_ERROR)
        return NULL;

    set_block_size(block_ptr, block_size);
    MARK_ALLOCATED(block_ptr);
    return KERNEL_TO_USER(block_ptr);
}

static size_t traversal(void *block_ptr)
{
    size_t left_cnt, right_cnt;
    if(block_ptr == NULL) return 0;
    left_cnt = traversal(LEFT_BLOCK(block_ptr));
    printf("ptr: %p, size: %u\n", block_ptr, BLOCK_SIZE(block_ptr));
    right_cnt = traversal(RIGHT_BLOCK(block_ptr));
    return left_cnt + 1 + right_cnt;
}

static int mm_check()
{
    void *block_ptr = (void *)((char *)mem_heap_lo() + 4);
    size_t freeblock_cnt = 0;
    while(block_ptr <= mem_heap_hi()) {
        size_t block_size = BLOCK_SIZE(block_ptr) & ~0x1;
        int is_allocated = IS_ALLOCATED(block_ptr);
        if(!is_allocated) freeblock_cnt++;
        printf("size: %u, alloc: %d\n", block_size, is_allocated);
        block_ptr = (void *)((char *)block_ptr + block_size);
    }
    return traversal(freeblock_root) == freeblock_cnt;
}
