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
    PageTableEntry *pgEntryPtr = (PageTableEntry *)(KB(4) + getPageEntryID(KERNEL_STACK_BASE-KB(4)) * PAGE_ENTRY_SIZE);
    freePhysicalPage(pgEntryPtr->pageBaseAddress);
    /* free user stack */
    for(pgEntryPtr = KB(8); pgEntryPtr < KB(12); pgEntryPtr += PAGE_ENTRY_SIZE) {
        if(!pgEntryPtr->present)
            continue;
        freePhysicalPage(pgEntryPtr->pageBaseAddress);
    }
    /*free page table */
    PageDirectoryEntry *pgdirEntryPtr = 0 * PAGE_ENTRY_SIZE;
    freePhysicalPage(pgdirEntryPtr->pageTableBaseAddress);
    pgdirEntryPtr = getPageDirectoryEntryID(USER_STACK_BASE-KB(4)) * PAGE_ENTRY_SIZE;
    freePhysicalPage(pgdirEntryPtr->pageTableBaseAddress);
    /* free page directory itself*/
    pgdirEntryPtr = 2 * PAGE_ENTRY_SIZE;
    freePhysicalPage(pgdirEntryPtr->pageTableBaseAddress);
    memset(0, 0, KB(12));
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

    /*
     * put the page needed to be free on the 0x0  
     * In order to get an virtual addr to access in a new pgdir
     */

    /* Copy the page directory */
    memcpy(0, VIRTUAL_PAGE_DIRECTORY_ADDR, KB(4));
    /* Copy the 0th page table, for kernel stack */
    memcpy(KB(4), MB(8), KB(4));
    /* Copy the xth page table, for user stack */
    memcpy(KB(8), MB(8) + KB(4) * getPageDirectoryEntryID(USER_STACK_BASE - MB(4)), KB(4));

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
