
/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using malloc and free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <xinu.h>

static void *extend_heap(uint32 words) {
    char *bp;
    uint32 size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    /* Initialize free block header/footer and the epilogue header */
    
    PUT(HDRP(bp), PACK(size, 0)); /* Free block header */
    PUT(FTRP(bp), PACK(size, 0)); /* Free block footer */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */

    /* Coalesce if the previous block was free include insert*/
    return coalesce(bp);
}
/* 
 * init - initialize the malloc package.
 */
int mm_init(void) {
//    printf("The ptr size of current machine platform is %d\n", sizeof(char *));
    struct procent *prptr = &proctab[currpid];
    prptr->heapIsInitialized = TRUE;
    int i;
    /* initiliaze prptr->seg_free_lists */
    for (i = 0; i < LISTMAX; i++) {
        prptr->seg_free_lists[i] = NULL;
    }
    /* Create the initial empty heap */
    if ((prptr->heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
        return -1;
    PUT(prptr->heap_listp, 0); /* Alignment padding */
    PUT(prptr->heap_listp + (1*WSIZE), PACK(DSIZE, 1)); /* Prologue header */
    PUT(prptr->heap_listp + (2*WSIZE), PACK(DSIZE, 1)); /* Prologue footer */
    PUT(prptr->heap_listp + (3*WSIZE), PACK(0, 1)); /* Epilogue header */
    prptr->heap_listp += (2*WSIZE);

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(INITCHUNKSIZE/WSIZE) == NULL)
        return -1;
    CHECKHEAP(1);
    return 0;
}

static void *place(void *bp, uint32 asize) {
    uint32 size = GET_SIZE(HDRP(bp));
    delete_node(bp);
    // need at least three DSIZE
    // one DSIZE for header and footer. two DSIZE separately for two pointer in free list
    if ((size - asize) < (3*DSIZE)) { //ignore the memory fragment
        PUT(HDRP(bp),PACK(size,1));
        PUT(FTRP(bp),PACK(size,1));
    } else if (asize >= 96) { // larger block is placed in the latter place in the list
        PUT(HDRP(bp),PACK(size - asize,0));
        PUT(FTRP(bp),PACK(size - asize,0));
        PUT(HDRP(NEXT_BLKP(bp)),PACK(asize,1));
        PUT(FTRP(NEXT_BLKP(bp)),PACK(asize,1));
        insert_node(bp,size - asize);
        return NEXT_BLKP(bp);
    } else { // otherwise, small block in placed in the front of the list
        PUT(HDRP(bp),PACK(asize,1));
        PUT(FTRP(bp),PACK(asize,1));
        PUT(HDRP(NEXT_BLKP(bp)),PACK(size - asize,0));
        PUT(FTRP(NEXT_BLKP(bp)),PACK(size - asize,0));
        insert_node(NEXT_BLKP(bp),size - asize);
    }
    return bp;
}
// inset a free block into free list
static void insert_node(void *bp, uint32 size)
{
    struct procent *prptr = &proctab[currpid];
    int tar = 0;
    uint32 j;
    for (j = size; (j > 1) && (tar < LISTMAX - 1); ) {
        j >>= 1;
        tar++;
    }
    char *i = prptr->seg_free_lists[tar];
    char *pre = NULL;
    while ((i != NULL) && (size > GET_SIZE(HDRP(i)))) {
        pre = i;
        i = SUCC(i);
    }// i should be first node size >= input size
    // empty list and establish a new list
    if (i == NULL && pre == NULL) {
        prptr->seg_free_lists[tar] = bp;
        SET_PTR(PRED_PTR(bp), NULL);
        SET_PTR(SUCC_PTR(bp), NULL); 
    } else if (i == NULL && pre != NULL) { // add at the last
        SET_PTR(PRED_PTR(bp), pre);
        SET_PTR(SUCC_PTR(bp), NULL);
        SET_PTR(SUCC_PTR(pre), bp);   
    } else if (pre == NULL) { // add at the first
        prptr->seg_free_lists[tar] = bp;
        SET_PTR(PRED_PTR(i), bp);
        SET_PTR(SUCC_PTR(bp), i);
        SET_PTR(PRED_PTR(bp), NULL);
    } else {
        SET_PTR(PRED_PTR(bp), pre);
        SET_PTR(SUCC_PTR(bp), i);
        SET_PTR(PRED_PTR(i), bp);
        SET_PTR(SUCC_PTR(pre), bp);
    }
}

static void delete_node(void *bp)
{
    struct procent *prptr = &proctab[currpid];
    uint32 size = GET_SIZE(HDRP(bp)), j;
    int tar = 0;
    for (j = size; (j > 1) && (tar < LISTMAX - 1); j >>= 1) {
        tar++;
    }
    // the object is the first one
    if (PRED(bp) == NULL) { 
        prptr->seg_free_lists[tar] = SUCC(bp);
        if (SUCC(bp) != NULL)
            SET_PTR(PRED_PTR(SUCC(bp)), NULL);
    } else if (SUCC(bp) == NULL) {
        // the object is the last one
        SET_PTR(SUCC_PTR(PRED(bp)), NULL);
    } else {
        // other circumstance
        SET_PTR(SUCC_PTR(PRED(bp)), SUCC(bp));
        SET_PTR(PRED_PTR(SUCC(bp)), PRED(bp));
    }
}

/* 
 * malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
syscall *malloc(uint32 size)
{
    struct procent *prptr = &proctab[currpid];
    if(prptr->heapIsInitialized == FALSE) 
        mm_init();
    uint32 asize, search; /* Adjusted block size */
    uint32 extendsize; /* Amount to extend heap if no fit */
    char *bp = NULL;
    
    /* Ignore spurious requests */
    if (size == 0)
        return NULL;
    
    /* Adjust block size to include overhead and alignment reqs. */
    asize = get_asize(size);
    search = asize;
    int target;
    for (target = 0; target < LISTMAX; target++, search >>= 1) {
        /* find target seg_free_list*/
        if ((search > 1) || (prptr->seg_free_lists[target] == NULL)) continue;
        char *i = prptr->seg_free_lists[target];
        for(;i != NULL;i = SUCC(i))
        {
            if (GET_SIZE(HDRP(i)) < asize) continue;
            bp = i;
            break;
        }
        if (bp != NULL) break;
    }
    if (bp == NULL) {
        /* No fit found. Get more memory and place the block */
        extendsize = MAX(asize,CHUNKSIZE);
        if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
            return NULL;
    }
    bp = place(bp, asize);
    CHECKHEAP(1);
    return bp;
}

static void *coalesce(void *bp)
{
    uint32 prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(bp)));
    uint32 next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    uint32 size = GET_SIZE(HDRP(bp));
    
    /* next is free */
    if (prev_alloc && !next_alloc) { 
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        delete_node(NEXT_BLKP(bp));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size,0));
    }
    /* pre is free */
    else if (!prev_alloc && next_alloc) {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        delete_node(PREV_BLKP(bp));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    /* both are allocated */
    else if (!prev_alloc && !next_alloc){ 
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        delete_node(PREV_BLKP(bp));
        delete_node(NEXT_BLKP(bp));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    insert_node(bp,size);
    return bp;
}
/*
 * free - Freeing a block does nothing.
 */
syscall free(void *bp) {
    if(bp == NULL) return;
    if(proctab[currpid].heapIsInitialized == FALSE) 
        mm_init();
    uint32 size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
    CHECKHEAP(1);
}

/*
 * realloc - Implemented simply in terms of malloc and free
 */
syscall *realloc(void *ptr, uint32 size) {
    if(proctab[currpid].heapIsInitialized == FALSE) 
        mm_init();
    if (ptr == NULL)
       return malloc(size);
    if (size == 0) {
       free(ptr);
       return NULL;
    }

    void *newptr;
    uint32 asize, oldsize;
    oldsize = GET_SIZE(HDRP(ptr));
    asize = get_asize(size);
    if(oldsize<asize) {
        int isnextFree;
        char *bp = realloc_coalesce(ptr,asize,&isnextFree);
        if(isnextFree==1){
            // printf("next block is free---------------\n");
            realloc_place(bp,asize);
        } else if(isnextFree ==0 && bp != ptr){ 
            // printf("previous block is free, move the point to new address,and move the payload--------\n");
            // must use memmove instead of memcpy
            memmove(bp, ptr, size);
            realloc_place(bp,asize);
        }else{
        /*realloc_coalesce is fail
        both directions are allocated*/
            newptr = malloc(size);
            memcpy(newptr, ptr, size);
            free(ptr);
            // printf("realloc coalesce is fail-----------------\n");
            CHECKHEAP(1);
            return newptr;
        }
        CHECKHEAP(1);
        return bp;
    }else if(oldsize>asize){/*just change the size of ptr*/
        realloc_place(ptr,asize);
        CHECKHEAP(1);
        return ptr;
    }
    CHECKHEAP(1);
    return ptr;
}
/* get the alignment size */
static uint32 get_asize(uint32 size)  {
    uint32 asize;
    if(size <= DSIZE){
        asize = 2 * (DSIZE);
    }else{
        asize = ALIGN(size + DSIZE);
    }
    return asize;
}

static void *realloc_coalesce(void *bp,uint32 newSize,int *isNextFree) {
    uint32  prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    uint32  next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    uint32 size = GET_SIZE(HDRP(bp));
    // flag: whether the next block is free
    *isNextFree = 0;
    /* coalesce the block and change the point */ 
    if(prev_alloc && !next_alloc) { // the next block is free
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        if(size>=newSize)
        {
            delete_node(NEXT_BLKP(bp));
            PUT(HDRP(bp), PACK(size,1));
            PUT(FTRP(bp), PACK(size,1));
            *isNextFree = 1;
        }
    } else if(!prev_alloc && next_alloc) { // the previous block is free 
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        if(size>=newSize)
        {
            delete_node(PREV_BLKP(bp));
            PUT(FTRP(bp),PACK(size,1));
            PUT(HDRP(PREV_BLKP(bp)),PACK(size,1));
            bp = PREV_BLKP(bp);
        }
    } else if(!prev_alloc && !next_alloc) { // both are free
        size +=GET_SIZE(FTRP(NEXT_BLKP(bp)))+ GET_SIZE(HDRP(PREV_BLKP(bp)));
        if(size>=newSize) {
            delete_node(PREV_BLKP(bp));
            delete_node(NEXT_BLKP(bp));
            PUT(FTRP(NEXT_BLKP(bp)),PACK(size,1));
            PUT(HDRP(PREV_BLKP(bp)),PACK(size,1));
            bp = PREV_BLKP(bp);
        }
    }
    return bp;
}
static void realloc_place(void *bp,uint32 asize) {
    uint32 csize = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp),PACK(csize,1));
    PUT(FTRP(bp),PACK(csize,1));
}

/* below code if for check heap invarints */
static void printblock(void *bp)  {
    long int hsize, halloc, fsize, falloc;

    hsize = GET_SIZE(HDRP(bp));
    halloc = GET_ALLOC(HDRP(bp));  
    fsize = GET_SIZE(FTRP(bp));
    falloc = GET_ALLOC(FTRP(bp));  

    if (hsize == 0) {
        printf("%p: EOL\n", bp);
        return;
    }
    printf("%p: header: [%ld:%c] footer: [%ld:%c]\n", bp, hsize, (halloc ? 'a' : 'f'), fsize, (falloc ? 'a' : 'f')); 
}

static int checkblock(void *bp)  {
    //area is aligned
    if ((uint32)bp % 8)
        printf("Error: %p is not doubleword aligned\n", bp);
    //header and footer match
    if (GET(HDRP(bp)) != GET(FTRP(bp)))
        printf("Error: header does not match footer\n");
    uint32 size = GET_SIZE(HDRP(bp));
    //size is valid
    if (size % 8)
       printf("Error: %p payload size is not doubleword aligned\n", bp);
    return GET_ALLOC(HDRP(bp));
}

static void printlist(void *i, long size)  {
    long int hsize, halloc;

    for(;i != NULL;i = SUCC(i)) {
        hsize = GET_SIZE(HDRP(i));
        halloc = GET_ALLOC(HDRP(i));
        printf("[listnode %ld] %p: header: [%ld:%c] prev: [%p]  next: [%p]\n", size, i, hsize, (halloc ? 'a' : 'f'), PRED(i), SUCC(i)); 
    }
}
static void checklist(void *i, uint32 tar) {
    void *pre = NULL;
    long int hsize, halloc;
    for(;i != NULL;i = SUCC(i)){
        if (PRED(i) != pre) 
            printf("Error: pred point error\n");
        if (pre != NULL && SUCC(pre) != i) 
            printf("Error: succ point error\n");
        hsize = GET_SIZE(HDRP(i));
        halloc = GET_ALLOC(HDRP(i));
        if (halloc)     
            printf("Error: list node should be free\n");
        if (pre != NULL && (GET_SIZE(HDRP(pre)) > hsize)) 
           printf("Error: list size order error\n");
        if (hsize < tar || ((tar != (1<<15)) && (hsize > (tar << 1)-1)))
           printf("Error: list node size error\n");
        pre = i;
    }
}
/* 
 * mm_checkheap - Check the heap for correctness
 */
void mm_checkheap(int verbose)  { 
    checkheap(verbose);
}
//heap level
void checkheap(int verbose) {
    struct procent *prptr = &proctab[currpid];
    char *bp = prptr->heap_listp;
    
    if (verbose)
        printf("Heap (%p):\n", prptr->heap_listp);
    // check head
    if ((GET_SIZE(HDRP(prptr->heap_listp)) != DSIZE) || !GET_ALLOC(HDRP(prptr->heap_listp)))
        printf("Bad prologue header\n");
    // block level
    checkblock(prptr->heap_listp);
    int pre_free = 0;
    
    for (bp = prptr->heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        if (verbose) 
            printblock(bp);
        int cur_free = checkblock(bp);
        // no contiguous free blocks
        if (pre_free && cur_free) {
            printf("Contiguous free blocks\n");
        }
   
    }
    // check the free list
    int i = 0, tarsize = 1;
    for (; i < LISTMAX; i++) {
        if (verbose) 
            printlist(prptr->seg_free_lists[i], tarsize);
        checklist(prptr->seg_free_lists[i],tarsize);
        tarsize <<= 1;
    }
    // check the tail
    if (verbose)
        printblock(bp);
    if ((GET_SIZE(HDRP(bp)) != 0) || !(GET_ALLOC(HDRP(bp))))
        printf("Bad epilogue header\n");
}
