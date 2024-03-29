/* kill.c - kill */

#include <xinu.h>

// void freeStack(uint32 stkBase, uint32 size) {
//     size = (uint32)roundmb(size);
//     uint32 addr = stkBase + sizeof(uint32) - size;
//     freeVirtualAddr(addr, size);
//     return;
// }

void freeLastProcessMemmoryResource() {
    /* free kernel stack */
    DBG_PRINTF("*******************free kernel stack*******************\n");
    PageTableEntry *pgEntryPtr = (PageTableEntry *)(KB(4) + getPageEntryID(VM_KERNEL_STACK_BASE-KB(4)) * PAGE_ENTRY_SIZE);
    freePhysicalPage(((pgEntryPtr->pageBaseAddress) << PAGE_OFFSET_BIT));
    /* free user stack */
    DBG_PRINTF("*******************free user stack*******************\n");
    for(pgEntryPtr = KB(8); pgEntryPtr < KB(12); pgEntryPtr += 1) {
        if(!(pgEntryPtr->present))
            continue;
        freePhysicalPage((pgEntryPtr->pageBaseAddress) << PAGE_OFFSET_BIT);
    }
    /*free page table */
    DBG_PRINTF("*******************free page table*******************\n");
    PageDirectoryEntry *pgdirEntryPtr = 0 * PAGE_ENTRY_SIZE;
    freePhysicalPage((pgdirEntryPtr->pageTableBaseAddress) << PAGE_OFFSET_BIT);
    pgdirEntryPtr = getPageDirectoryEntryID(VM_USER_STACK_BASE-KB(4)) * PAGE_ENTRY_SIZE;
    freePhysicalPage((pgdirEntryPtr->pageTableBaseAddress) << PAGE_OFFSET_BIT);
    /* free page directory itself*/
    DBG_PRINTF("*******************free page diretory*******************\n");
    pgdirEntryPtr = 2 * PAGE_ENTRY_SIZE;
    freePhysicalPage((pgdirEntryPtr->pageTableBaseAddress) << PAGE_OFFSET_BIT);
    memset(0, 0, KB(12));
    DBG_PRINTF("\n*******************finish free child process's memory resource*******************\n");
    return;
}

/*------------------------------------------------------------------------
 *  kill  -  Kill a process and remove it from the system
 *------------------------------------------------------------------------
 */
syscall	kill(
	  pid32		pid		        /* ID of process to kill	    */
	)
{
	intmask	mask;			    /* Saved interrupt mask		    */
	struct	procent *prptr;		/* Ptr to process's table entry	*/
	int32	i;			        /* Index into descriptors	    */

	mask = disable();
	if (isbadpid(pid) || (pid == NULLPROC)
	    || ((prptr = &proctab[pid])->prstate) == PR_FREE) {
		restore(mask);
		return SYSERR;
	}

	if (--prcount <= 1) {		/* Last user process completes	*/
		xdone();
	}

	send(prptr->prparent, pid);
	for (i=0; i<3; i++) {
		close(prptr->prdesc[i]);
	}
    // free the heap space
    mem_free();
    /*
     * put the page needed to be free on the 0x0  
     * In order to get an virtual addr to access in a new pgdir
     */

    /* Copy the page directory */
    memcpy(0, VM_PAGE_DIRECTORY_ADDR, KB(4));
    /* Copy the 0th page table, for kernel stack */
    memcpy(KB(4), MB(8), VM_PAGE_SIZE);
    /* Copy the xth page table, for user stack */
    memcpy(KB(8), MB(8) + VM_PAGE_SIZE * getPageDirectoryEntryID(VM_USER_STACK_BASE - KB(4)), VM_PAGE_SIZE);

	switch (prptr->prstate) {
	case PR_CURR:
		prptr->prstate = PR_FREE;	/* Suicide */
		resched(1);

	case PR_SLEEP:
	case PR_RECTIM:
		unsleep(pid);
		prptr->prstate = PR_FREE;
		break;

	case PR_WAIT:
		semtab[prptr->prsem].scount++;
		/* Fall through */

	case PR_READY:
		getitem(pid);		/* Remove from queue */
		/* Fall through */

	default:
		prptr->prstate = PR_FREE;
	}

	restore(mask);
	return OK;
}
