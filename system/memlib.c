/*
 * memlib.c - a module that simulates the memory system.  Needed because it 
 *            allows us to interleave calls from the student's malloc package 
 *            with the system's malloc package in libc.
 */
#include <xinu.h>

/* private variables */
static char *mem_start_brk;  /* points to first byte of heap */
static char *mem_brk;        /* points to last byte of heap */
static char *mem_max_addr;   /* largest legal heap address */ 

#define MAX_HEAP MB(20)  
/* 
 * mem_init - initialize the memory system model
 */
void mem_init() {
    mem_start_brk = VM_HEAP_BASE;
    mem_max_addr = mem_start_brk + MAX_HEAP;  /* max legal heap address */
    mem_brk = mem_start_brk;                  /* heap is empty initially */
}

void mem_free() {
    struct procent *prptr = &proctab[currpid];
    freeVirtualAddr(prptr->pageDirectory, mem_start_brk, mem_brk-mem_start_brk);
    mem_start_brk = mem_brk = 0;
    prptr->heapSize = 0;
    return;
}
/* 
 * mem_sbrk - simple model of the sbrk function. Extends the heap 
 *    by incr bytes and returns the start address of the new area. In
 *    this model, the heap cannot be shrunk.
 */
void *mem_sbrk(int32 delta)  {
    void *old_brk = mem_brk;
    struct procent *prptr = &proctab[currpid];

    if (delta < 0) {
        kprintf("ERROR: heap size cannot be shunk\n");
        return (void *)-1;
    }

    uint32 newHeapSize = prptr->heapSize + delta;
    if(newHeapSize > MAX_HEAP) {
        kprintf("ERROR: mem_sbrk failed. Ran out of memory...\n");
        return (void *)-1;
    }

    mem_brk += delta;
    allocateVirtualAddr(prptr->pageDirectory, old_brk, delta, 1);
    
    prptr->heapSize = newHeapSize;
    return old_brk;

}

/*
 * mem_heap_lo - return address of the first heap byte
 */
void *mem_heap_lo()
{
    return (void *)mem_start_brk;
}

/* 
 * mem_heap_hi - return address of last heap byte
 */
void *mem_heap_hi()
{
    return (void *)(mem_brk - 1);
}

/*
 * mem_heapsize() - returns the heap size in bytes
 */
uint32 mem_heapsize()  {
    return (uint32)(mem_brk - mem_start_brk);
}

/*
 * mem_pagesize() - returns the page size of the system
 */
uint32 mem_pagesize() {
    return VM_PAGE_SIZE;
}
