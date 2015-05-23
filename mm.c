/* *
 * mm.c
 *
 * 각 block은 앞뒤로 각각 4-byte의 boundary tag를 가지고 있으며,
 * free blocks만으로 구성된 explicit binary search tree로 관리한다.
 * BST는 splay tree로서 대략적으로 balance를 유지한다.
 * 각 node는 left/right child와 parent에 대한 pointer를 가지고 있다.
 *
 * Block size를 20MB 이하로 제한하면 25-bit에 담을 수 있고,
 * block에 대한 pointer도 offset으로 저장하면 25-bit에 담을 수 있다.
 * 이렇게 하면, 최소 block size를 16-byte로 할 수 있다.
 *
 * Free block의 bit layout은 다음과 같다.
 * [  0,   1): is_allocated
 * [  1,  26): block_size
 * [ 26,  51): left_child
 * [ 51,  52): left_or_right
 * [ 52,  77): parent
 * [ 77, 102): right_child
 * ...
 * [102, 103): is_allocated
 * [103, 128): block_size
 *
 * Allocated block은 boundary tag만을 가지고 있다.
 * is_allocated, block_size는 free block의 layout과 같다.
 * */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>

#include "mm.h"
#include "memlib.h"

// MM_CHECK를 켜면 mm_check가 정의되고
// 매 mm_malloc, mm_free, mm_realloc이 끝나기 직전에
// mm_check를 불러 assert 하도록 되어 있다.
#ifdef MM_CHECK
static int mm_check(void);
#endif

// student information
student_t student = {
    "심규민",
    "2009-11744"
};

// global variables
static void *freeblock_base;
static void *freeblock_root;

// declaration of static functions related with free block tree
static void splay_freeblock(void *block_ptr);
static void insert_freeblock(void *block_ptr);
static void remove_freeblock(void *block_ptr);
static void *find_freeblock(size_t block_size);
static void *coalesce_freeblock(void *block_ptr);

// constants for brk operations
#define BRK_OFFSET 4
#define SBRK_ERROR ((void *)-1)
// macros for boundary tag addition/deletion
#define USER_TO_KERNEL(P) ((void *)((char *)(P) - 4))
#define KERNEL_TO_USER(P) ((void *)((char *)(P) + 4))
// constants and a macro related with block size
#define BOUNDARY_SIZE 8
#define ALIGNED_SIZE(SZ) ((((SZ) + 7) & ~7) + BOUNDARY_SIZE)
#define MIN_BLOCK_SIZE 16
// macros for bit-wise operations or pointer operations
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
// NIL is NULL pointer
#define NIL BASE_PLUS(ONES(25))
// constants for is_allocated
#define FREED 0
#define ALLOCATED 1
// constants for left_or_right
#define LEFT 0
#define RIGHT 1
#define LR_WHATEVER 0
// macros for getting bit-encoded value
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
// macros for operating adjacent blocks
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
// macros for setting bit-encoded value
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

// PARENT block을 CHILD block의 parent로 설정하고,
// CHILD block을 PARENT block의 LR child로 설정한다.
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

// 전역 변수 등을 초기화 한다.
// 성공하면 0을 리턴하고, 실패하면 -1을 리턴한다.
// 8-byte align을 맞추기 위하여 heap의 맨 앞을
// BRK_OFFSET(4-byte)만큼은 사용하지 않는다.
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

// SIZE를 담을 수 있는 8-byte align 된 block을 할당하여 리턴한다.
static void *mm_malloc_(size_t size)
{
    size_t block_size = ALIGNED_SIZE(size);
    void *block_ptr = find_freeblock(block_size);

    if(block_ptr == NIL) {
        // 기존의 free block에서 할당할 수 없는 경우
        block_ptr = mem_sbrk(block_size);
        if(block_ptr == SBRK_ERROR)
            return NULL;
        SET_BLOCK_SIZE(block_ptr, block_size);
    }else {
        // 기본의 free block에서 할당할 수 있는 경우
        remove_freeblock(block_ptr);
        if(BLOCK_SIZE(block_ptr) >= block_size + MIN_BLOCK_SIZE) {
            // free block을 아껴서 나눠 쓸 수 있는 경우
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

// PTR에 할당된 block을 해제한다.
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

// PTR에 할당된 block을 새로운 SIZE로 변경한다.
static void *mm_realloc_(void *ptr, size_t size)
{
    size_t aligned_size = ALIGNED_SIZE(size);
    void *block_ptr = USER_TO_KERNEL(ptr), *new_block_ptr;
    size_t block_size = BLOCK_SIZE(block_ptr);

    if(aligned_size <= block_size) {
        // shrink: 크기가 줄어드는 경우
        if(aligned_size + MIN_BLOCK_SIZE <= block_size) {
            // 아껴서 나눠 쓰기
            new_block_ptr = SHIFT(block_ptr, aligned_size);
            SET_BLOCK_SIZE(block_ptr, aligned_size);
            SET_IS_ALLOCATED(block_ptr, ALLOCATED);
            SET_BLOCK_SIZE(new_block_ptr, block_size - aligned_size);
            SET_IS_ALLOCATED(new_block_ptr, FREED);
            insert_freeblock(new_block_ptr);
        }
    }else {
        // expand: 크기가 늘어나는 경우
        size_t prev_size = 0, next_size = 0, total_size;
        if(block_ptr > freeblock_base && PREV_IS_ALLOCATED(block_ptr) == FREED)
            prev_size = PREV_BLOCK_SIZE(block_ptr);
        if(NEXT_BLOCK(block_ptr) <= mem_heap_hi() && NEXT_IS_ALLOCATED(block_ptr) == FREED)
            next_size = NEXT_BLOCK_SIZE(block_ptr);
        total_size = prev_size + block_size + next_size;
        if(aligned_size <= total_size) {
            // in-place: 앞뒤 block을 합치면 fit 하는 경우
            if(next_size > 0) {
                void *next = NEXT_BLOCK(block_ptr);
                remove_freeblock(next);
            }
            if(prev_size > 0) {
                void *prev = PREV_BLOCK(block_ptr);
                remove_freeblock(prev);
                memmove(KERNEL_TO_USER(prev), ptr, block_size - BOUNDARY_SIZE);
                block_ptr = prev;
            }
            if(aligned_size + MIN_BLOCK_SIZE <= total_size) {
                // 아껴서 나눠 쓰기
                new_block_ptr = SHIFT(block_ptr, aligned_size);
                SET_BLOCK_SIZE(block_ptr, aligned_size);
                SET_IS_ALLOCATED(block_ptr, ALLOCATED);
                SET_BLOCK_SIZE(new_block_ptr, total_size - aligned_size);
                SET_IS_ALLOCATED(new_block_ptr, FREED);
                insert_freeblock(new_block_ptr);
            }else {
                SET_BLOCK_SIZE(block_ptr, total_size);
                SET_IS_ALLOCATED(block_ptr, ALLOCATED);
            }
        }else {
            // out-place: 앞뒤 block을 합쳐도 안되는 경우
            void *new_ptr = mm_malloc_(size);
            if(new_ptr != NULL) {
                memcpy(new_ptr, ptr, block_size - BOUNDARY_SIZE);
                block_ptr = USER_TO_KERNEL(new_ptr);
            }
            mm_free_(ptr);
            if(new_ptr == NULL)
                return NULL;
        }
    }

    return KERNEL_TO_USER(block_ptr);
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

// BST의 balancing을 위한 splaying.
// X를 root로 만든다.
static void splay_freeblock(void *x)
{
    void *p, *g;
    int x_lr, p_lr, g_lr;

    if(x == NIL)
        return;

    x_lr = LEFT_OR_RIGHT(x);
    p = PARENT(x);

    if(p == NIL)
        return;

    p_lr = LEFT_OR_RIGHT(p);
    g = PARENT(p);

    if(g == NIL) {
        // zig
        if(x_lr == LEFT) {
            set_link(p, RIGHT_CHILD(x), LEFT);
            set_link(x, p, RIGHT);
        }else {
            set_link(p, LEFT_CHILD(x), RIGHT);
            set_link(x, p, LEFT);
        }
        set_link(NIL, x, LR_WHATEVER);
        return;
    }

    g_lr = LEFT_OR_RIGHT(g);
    set_link(PARENT(g), x, g_lr);

    if(x_lr == p_lr) {
        // zig-zig
        if(x_lr == LEFT) {
            set_link(g, RIGHT_CHILD(p), LEFT);
            set_link(p, RIGHT_CHILD(x), LEFT);
            set_link(x, p, RIGHT);
            set_link(p, g, RIGHT);
        }else {
            set_link(g, LEFT_CHILD(p), RIGHT);
            set_link(p, LEFT_CHILD(x), RIGHT);
            set_link(x, p, LEFT);
            set_link(p, g, LEFT);
        }
    }else {
        // zig-zag
        if(x_lr == LEFT) {
            set_link(p, RIGHT_CHILD(x), LEFT);
            set_link(g, LEFT_CHILD(x), RIGHT);
            set_link(x, p, RIGHT);
            set_link(x, g, LEFT);
        }else {
            set_link(p, LEFT_CHILD(x), RIGHT);
            set_link(g, RIGHT_CHILD(x), LEFT);
            set_link(x, p, LEFT);
            set_link(x, g, RIGHT);
        }
    }

    splay_freeblock(x);
}

// Splay tree에 주어진 free block을 넣는다.
static void insert_freeblock(void *block_ptr)
{
    size_t block_size = BLOCK_SIZE(block_ptr);
    void *p = freeblock_root;
    int lr = LR_WHATEVER;

    SET_LEFT_CHILD(block_ptr, NIL);
    SET_RIGHT_CHILD(block_ptr, NIL);

    // 위치 찾기
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

    // 찾은 위치에 넣기
    set_link(p, block_ptr, lr);

    // balancing
    splay_freeblock(block_ptr);
}

// Splay tree에서 주어진 free block을 뺀다.
static void remove_freeblock(void *block_ptr)
{
    void *left = LEFT_CHILD(block_ptr);
    void *right = RIGHT_CHILD(block_ptr);
    void *parent = PARENT(block_ptr);
    int lr = LEFT_OR_RIGHT(block_ptr);
    void *target;

    // child가 하나 이하인 trivial case를 먼저 확인
    if(left == NIL && right == NIL) {
        target = NIL;
    }else if(left == NIL && right != NIL) {
        target = right;
    }else if(left != NIL && right == NIL) {
        target = left;
    }else {
        // child가 둘인 non-trivial case
        // 오른쪽 subtree의 left-most node로 바꿔치기 한다.
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

    // 지워진 block의 parent와 연결
    set_link(parent, target, lr);

    // balancing
    splay_freeblock(parent);
}

// 주어진 크기보다 크거나 같은 free block을 찾아서 리턴한다.
// 없으면 NIL을 리턴한다.
static void *find_freeblock(size_t block_size)
{
    void *p = freeblock_root, *fit = NIL, *pp = NIL;

    // 찾기
    while(p != NIL) {
        pp = p;
        if(block_size <= BLOCK_SIZE(p))
            fit = p;
        if(block_size < BLOCK_SIZE(p)) {
            p = LEFT_CHILD(p);
        }else {
            p = RIGHT_CHILD(p);
        }
    }

    // 마지막으로 접근한 node를 기준으로 balancing
    splay_freeblock(pp);

    return fit;
}

// 주어진 free block이 앞뒤의 free block과 합쳐질 수 있으면,
// 앞뒤의 free block들을 tree에서 제거하고,
// 주어진 free block과 합쳐서 그 결과를 리턴한다.
static void *coalesce_freeblock(void *block_ptr)
{
    void *cur = block_ptr;

    // 앞의 block과 합쳐질 수 있는지 확인 후 합치기
    if(cur > freeblock_base) {
        if(PREV_IS_ALLOCATED(cur) == FREED) {
            void *prev = PREV_BLOCK(cur);
            size_t size = BLOCK_SIZE(prev) + BLOCK_SIZE(cur);
            remove_freeblock(prev);
            SET_BLOCK_SIZE(prev, size);
            cur = prev;
        }
    }

    // 뒤의 block과 합쳐질 수 있는지 확인 후 합치기
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

// 주어진 BST를 순차적으로 돌아다니며,
// 각 node의 정보를 출력하고 전체 node의 개수를 리턴한다.
// BST가 제대로 구성되어 있는지 확인할 수 있다.
static size_t traversal(void *block_ptr)
{
    size_t left_cnt, right_cnt;
    if(block_ptr == NIL) return 0;
    left_cnt = traversal(LEFT_CHILD(block_ptr));
    printf("ptr: %p, size: %u\n", block_ptr, BLOCK_SIZE(block_ptr));
    right_cnt = traversal(RIGHT_CHILD(block_ptr));
    return left_cnt + 1 + right_cnt;
}

// Heap 영역의 blocks을 확인한다.
// 이상한 점이 발견되면 0을 리턴한다.
// 모든 blocks을 작은 주소부터 차례로 방문하여,
// 각 block의 정보를 출력하고 free block의 개수를 센다.
// 이렇게 센 free block의 개수와 tree의 node 개수가 같은지 리턴한다.
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
