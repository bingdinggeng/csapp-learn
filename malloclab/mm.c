/* 参考 wdxtub 的 Malloc Lab 笔记，链接见根目录 README.md。 */
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mm.h"
#include "memlib.h"

team_t team = { "malloclab", "LTM", "LTM", "", "" };

#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE 4096
#define LIST_COUNT 16
#define ALIGN(n) (((n) + 7) & ~(size_t)7)
#define MIN_BLOCK ALIGN(DSIZE + 2 * sizeof(void *))
#define PACK(n, a) ((uint32_t)(n) | (a))
#define GET(p) (*(uint32_t *)(p))
#define PUT(p, v) (*(uint32_t *)(p) = (uint32_t)(v))
#define GET_SIZE(p) (GET(p) & ~(uint32_t)7)
#define GET_ALLOC(p) (GET(p) & 1)
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE((char *)(bp) - DSIZE))
#define PREV_FREE(bp) (*(void **)(bp))
#define NEXT_FREE(bp) (*(void **)((char *)(bp) + sizeof(void *)))

static void **free_lists;
static char *heap_listp;
void *mm_calloc(size_t nmemb, size_t size);
void mm_checkheap(int line);

#ifdef MM_DEBUG
#define CHECK_HEAP() mm_checkheap(__LINE__)
#else
#define CHECK_HEAP() ((void)0)
#endif

static int class_index(size_t size)
{
    int index = 0;
    size_t bound = 32;
    while (index < LIST_COUNT - 1 && size > bound) {
        bound <<= 1;
        ++index;
    }
    return index;
}

static void write_block(void *bp, size_t size, int allocated)
{
    PUT(HDRP(bp), PACK(size, allocated));
    PUT(FTRP(bp), PACK(size, allocated));
}

/* 改大小前先摘链。 */
static void remove_free(void *bp)
{
    void *prev = PREV_FREE(bp);
    void *next = NEXT_FREE(bp);
    if (prev != NULL)
        NEXT_FREE(prev) = next;
    else
        free_lists[class_index(GET_SIZE(HDRP(bp)))] = next;
    if (next != NULL)
        PREV_FREE(next) = prev;
}

static void insert_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));
    int index = class_index(size);
    void *prev = NULL;
    void *next = free_lists[index];
    while (next != NULL && GET_SIZE(HDRP(next)) < size) {
        prev = next;
        next = NEXT_FREE(next);
    }
    PREV_FREE(bp) = prev;
    NEXT_FREE(bp) = next;
    if (prev != NULL)
        NEXT_FREE(prev) = bp;
    else
        free_lists[index] = bp;
    if (next != NULL)
        PREV_FREE(next) = bp;
}

static void *coalesce(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));
    void *next = NEXT_BLKP(bp);
    if (!GET_ALLOC(HDRP(next))) {
        remove_free(next);
        size += GET_SIZE(HDRP(next));
    }
    if (!GET_ALLOC((char *)bp - DSIZE)) {
        void *prev = PREV_BLKP(bp);
        remove_free(prev);
        size += GET_SIZE(HDRP(prev));
        bp = prev;
    }
    write_block(bp, size, 0);
    insert_free(bp);
    return bp;
}

static void *extend_heap(size_t size)
{
    void *bp;
    size = ALIGN(size);
    if (size < MIN_BLOCK)
        size = MIN_BLOCK;
    if (size > INT_MAX)
        return NULL;
    bp = mem_sbrk((int)size);
    if (bp == (void *)-1)
        return NULL;
    write_block(bp, size, 0);
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
    return coalesce(bp);
}

static size_t adjusted_size(size_t size)
{
    size_t result;
    if (size == 0 || size > (size_t)INT_MAX - DSIZE - 7)
        return 0;
    result = ALIGN(size + DSIZE);
    return result < MIN_BLOCK ? MIN_BLOCK : result;
}

static void *find_fit(size_t size)
{
    int index;
    for (index = class_index(size); index < LIST_COUNT; ++index) {
        void *bp;
        for (bp = free_lists[index]; bp != NULL; bp = NEXT_FREE(bp))
            if (GET_SIZE(HDRP(bp)) >= size)
                return bp;
    }
    return NULL;
}

static void split_allocated(void *bp, size_t wanted, size_t total)
{
    if (total - wanted >= MIN_BLOCK) {
        void *rest;
        write_block(bp, wanted, 1);
        rest = NEXT_BLKP(bp);
        write_block(rest, total - wanted, 0);
        coalesce(rest);
    } else {
        write_block(bp, total, 1);
    }
}

int mm_init(void)
{
    char *start;
    size_t heads_size = ALIGN(LIST_COUNT * sizeof(void *));
    free_lists = NULL;
    heap_listp = NULL;
    start = mem_sbrk((int)(heads_size + 4 * WSIZE));
    if (start == (void *)-1)
        return -1;
    free_lists = (void **)start;
    memset(free_lists, 0, heads_size);
    start += heads_size;
    PUT(start, 0);
    PUT(start + WSIZE, PACK(DSIZE, 1));
    PUT(start + 2 * WSIZE, PACK(DSIZE, 1));
    PUT(start + 3 * WSIZE, PACK(0, 1));
    heap_listp = start + 2 * WSIZE;
    if (extend_heap(CHUNKSIZE) == NULL)
        return -1;
    CHECK_HEAP();
    return 0;
}

void *mm_malloc(size_t size)
{
    size_t wanted = adjusted_size(size);
    size_t total;
    void *bp;
    if (wanted == 0)
        return NULL;
    bp = find_fit(wanted);
    if (bp == NULL) {
        bp = extend_heap(wanted > CHUNKSIZE ? wanted : CHUNKSIZE);
        if (bp == NULL)
            return NULL;
    }
    total = GET_SIZE(HDRP(bp));
    remove_free(bp);
    split_allocated(bp, wanted, total);
    CHECK_HEAP();
    return bp;
}

void mm_free(void *ptr)
{
    if (ptr == NULL)
        return;
    write_block(ptr, GET_SIZE(HDRP(ptr)), 0);
    coalesce(ptr);
    CHECK_HEAP();
}

void *mm_realloc(void *ptr, size_t size)
{
    size_t wanted, old_size, total;
    void *next, *newptr;
    if (ptr == NULL)
        return mm_malloc(size);
    if (size == 0) {
        mm_free(ptr);
        return NULL;
    }
    wanted = adjusted_size(size);
    if (wanted == 0)
        return NULL;
    old_size = GET_SIZE(HDRP(ptr));
    if (wanted <= old_size) {
        split_allocated(ptr, wanted, old_size);
        CHECK_HEAP();
        return ptr;
    }

    next = NEXT_BLKP(ptr);
    total = old_size;
    if (!GET_ALLOC(HDRP(next)))
        total += GET_SIZE(HDRP(next));
    if (total >= wanted) {
        remove_free(next);
        split_allocated(ptr, wanted, total);
        CHECK_HEAP();
        return ptr;
    }

    if (GET_SIZE(HDRP(next)) == 0 ||
        (!GET_ALLOC(HDRP(next)) &&
         GET_SIZE(HDRP(NEXT_BLKP(next))) == 0)) {
        size_t extra = wanted - total;
        if (mem_sbrk((int)extra) != (void *)-1) {
            if (total != old_size)
                remove_free(next);
            write_block(ptr, wanted, 1);
            PUT(HDRP(NEXT_BLKP(ptr)), PACK(0, 1));
            CHECK_HEAP();
            return ptr;
        }
    }

    /* 向前合并时数据可能重叠。 */
    if (!GET_ALLOC((char *)ptr - DSIZE)) {
        void *prev = PREV_BLKP(ptr);
        size_t combined = GET_SIZE(HDRP(prev)) + total;
        if (combined >= wanted) {
            remove_free(prev);
            if (total != old_size)
                remove_free(next);
            memmove(prev, ptr, old_size - DSIZE);
            split_allocated(prev, wanted, combined);
            CHECK_HEAP();
            return prev;
        }
    }
    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    memcpy(newptr, ptr, old_size - DSIZE);
    mm_free(ptr);
    CHECK_HEAP();
    return newptr;
}

void *mm_calloc(size_t nmemb, size_t size)
{
    void *ptr;
    size_t bytes;
    if (size != 0 && nmemb > SIZE_MAX / size)
        return NULL;
    bytes = nmemb * size;
    ptr = mm_malloc(bytes);
    if (ptr != NULL)
        memset(ptr, 0, bytes);
    return ptr;
}

static void heap_error(int line, const char *reason)
{
    fprintf(stderr, "mm_checkheap (caller line %d): %s\n", line, reason);
    exit(EXIT_FAILURE);
}

static int in_heap(const void *ptr, size_t bytes)
{
    uintptr_t p = (uintptr_t)ptr;
    uintptr_t lo = (uintptr_t)mem_heap_lo();
    size_t heap_size = mem_heapsize();
    return p >= lo && p - lo <= heap_size && bytes <= heap_size - (p - lo);
}

void mm_checkheap(int line)
{
    char *bp;
    size_t physical_free = 0, listed_free = 0;
    int previous_free = 0, index;
    if (heap_listp == NULL || free_lists == NULL)
        heap_error(line, "heap not initialized");
    if (!in_heap(free_lists, LIST_COUNT * sizeof(void *)) ||
        !in_heap(heap_listp - WSIZE, DSIZE) ||
        GET(HDRP(heap_listp)) != PACK(DSIZE, 1) ||
        GET(heap_listp) != PACK(DSIZE, 1))
        heap_error(line, "invalid prologue or list heads");

    for (bp = heap_listp + DSIZE; ; bp += GET_SIZE(HDRP(bp))) {
        size_t size;
        int allocated;
        if (!in_heap(HDRP(bp), WSIZE))
            heap_error(line, "header outside heap");
        size = GET_SIZE(HDRP(bp));
        allocated = GET_ALLOC(HDRP(bp));
        if (size == 0) {
            if (GET(HDRP(bp)) != PACK(0, 1) ||
                (uintptr_t)bp != (uintptr_t)mem_heap_lo() + mem_heapsize())
                heap_error(line, "invalid epilogue");
            break;
        }
        if (((uintptr_t)bp & 7) != 0 || size < MIN_BLOCK ||
            (GET(HDRP(bp)) & 6) != 0 || !in_heap(HDRP(bp), size))
            heap_error(line, "invalid alignment, size or bounds");
        if (GET(HDRP(bp)) != GET(FTRP(bp)))
            heap_error(line, "header/footer mismatch");
        if (!allocated) {
            if (previous_free)
                heap_error(line, "adjacent free blocks were not coalesced");
            ++physical_free;
        }
        previous_free = !allocated;
    }

    for (index = 0; index < LIST_COUNT; ++index) {
        void *prev = NULL;
        size_t previous_size = 0;
        for (bp = free_lists[index]; bp != NULL; bp = NEXT_FREE(bp)) {
            char *scan;
            size_t size;
            if (++listed_free > physical_free)
                heap_error(line, "free-list cycle or duplicate block");
            if (!in_heap(bp, 2 * sizeof(void *)) || ((uintptr_t)bp & 7))
                heap_error(line, "invalid free-list pointer");
            /* 排除指向块中间的指针。 */
            for (scan = heap_listp + DSIZE; GET_SIZE(HDRP(scan)) != 0;
                 scan = NEXT_BLKP(scan))
                if (scan == bp)
                    break;
            if (scan != bp || GET_ALLOC(HDRP(bp)))
                heap_error(line, "list entry is not a free block");
            size = GET_SIZE(HDRP(bp));
            if (class_index(size) != index || size < previous_size)
                heap_error(line, "wrong size class or unsorted list");
            if (PREV_FREE(bp) != prev)
                heap_error(line, "inconsistent previous/next links");
            prev = bp;
            previous_size = size;
        }
    }
    if (listed_free != physical_free)
        heap_error(line, "free blocks missing from lists");
}
