/*
 * memlib.c - a module that simulates the memory system.  Needed because it 
 *            allows us to interleave calls from the student's malloc package 
 *            with the system's malloc package in libc.
 */
#include <xinu.h>

void mem_free() {
    struct procent *prptr = &proctab[currpid];
    freeVirtualAddr(prptr->pageDirectory, VM_HEAP_BASE, prptr->heapSize);
    prptr->heapSize = 0;
    return;
}
/* 
 * mem_sbrk - simple model of the sbrk function. Extends the heap 
 *    by incr bytes and returns the start address of the new area. In
 *    this model, the heap cannot be shrunk.
 */
void *mem_sbrk(int32 delta)  {
    if (delta < 0) {
        kprintf("ERROR: heap size cannot be shunk\n");
        return (void *)-1;
    }

    struct procent *prptr = &proctab[currpid];
    void *old_brk = VM_HEAP_BASE + prptr->heapSize;

    uint32 newHeapSize = prptr->heapSize + delta;
    if(newHeapSize > VM_MAX_HEAP_SIZE) {
        kprintf("ERROR: mem_sbrk failed. Ran out of memory...\n");
        return (void *)-1;
    }

    allocateVirtualAddr(prptr->pageDirectory, old_brk, delta, 1);
    
    prptr->heapSize = newHeapSize;
    return old_brk;

}

/*
 * mem_heap_lo - return address of the first heap byte
 */
void *mem_heap_lo()
{
    return (void *)VM_HEAP_BASE;
}

/* 
 * mem_heap_hi - return address of last heap byte
 */
void *mem_heap_hi()
{
    return (void *)(VM_HEAP_BASE + mem_heapsize() - 1);
}

/*
 * mem_heapsize() - returns the heap size in bytes
 */
uint32 mem_heapsize()  {
    return proctab[currpid].heapSize;
}

/*
 * mem_pagesize() - returns the page size of the system
 */
uint32 mem_pagesize() {
    return VM_PAGE_SIZE;
}
