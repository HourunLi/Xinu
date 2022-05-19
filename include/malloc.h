#pragma once
#include <kernel.h>

extern int mm_init(void);
extern syscall *malloc(uint32 size);
#define syscall_malloc(...) \
		do_generic_syscall(syscall, SYSCALL_MALLOC, __VA_ARGS__)
extern syscall free(void *ptr);
#define syscall_free(...) \
		do_generic_syscall(syscall, SYSCALL_FREE, __VA_ARGS__)
extern syscall *realloc(void *ptr, uint32 size);
#define syscall_realloc(...) \
		do_generic_syscall(syscall, SYSCALL_REALLOC, __VA_ARGS__)

// #define DEBUG 1
/* If you want debugging output, use the following macro.  When you hand
 * in, remove the #define DEBUG line. */
#ifdef DEBUG
    #define DBG_PRINTF(...) printf(__VA_ARGS__)
    #define CHECKHEAP(verbose) mm_checkheap(verbose)
#else
    #define DBG_PRINTF(...)
    #define CHECKHEAP(verbose)
#endif


/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */

#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define LISTMAX 16

/* Basic constants and macros */
#define WSIZE 4 /* Word and header/footer size (bytes) */
#define DSIZE 8 /* Double word size (bytes) */
#define CHUNKSIZE (1<<12) /* Extend heap by this amount (bytes) */
#define INITCHUNKSIZE (1<<6)

#define MAX(x, y) ((x) > (y)? (x) : (y))
#define MIN(x, y) ((x) < (y)? (x) : (y))
/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/* for explict linkedlist */
#define PRED_PTR(ptr) ((char *)(ptr))
#define SUCC_PTR(ptr) ((char *)(ptr) + DSIZE)

/* get the ptr*/
#define PRED(ptr) (*(char **)(ptr))
#define SUCC(ptr) (*(char **)(SUCC_PTR(ptr)))

/* set the ptr */
#define SET_PTR(p, ptr) (*(char **)(p) = (char *)ptr)

static void *extend_heap(uint32 words);
static void *place(void *bp, uint32 asize);
static void *coalesce(void *bp);
static void insert_node(void *bp, uint32 size);
static void delete_node(void *bp);
static uint32 get_asize(uint32 size);
static void *realloc_coalesce(void *bp,uint32 newSize,int *isNextFree);
static void realloc_place(void *bp,uint32 asize);
void checkheap(int verbose);
void mm_checkheap(int verbose);