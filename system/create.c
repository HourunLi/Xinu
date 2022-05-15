/* create.c - create, newpid */

#include <xinu.h>
#include <string.h>
local	int newpid();
extern void ret_k2u();
extern taskstate tss[];
/*------------------------------------------------------------------------
 *  create  -  Create a process to start running a function on x86
 *------------------------------------------------------------------------
 */

void initializeTable0(PageTable tableAddr, Page kernelStack_phy) {
    uint32 virtualAddr = 0, physicalAddr = 0;
    uint32 pageTableEntryID;
    for(; physicalAddr < (uint32)&end; virtualAddr += KB(4), physicalAddr += KB(4)) {
        pageTableEntryID = (virtualAddr >> PAGE_OFFSET_BIT) & ((1 << PAGE_TABLE_BIT)-1);
        uint32 userSupervisor = 0, present = 1;
        if(MB(1) <= physicalAddr && physicalAddr < (uint32)&end) {
            userSupervisor = 1;
        }
        /* Initialize the first page */
        initializePageTableEntry(tableAddr, pageTableEntryID, physicalAddr, present, userSupervisor);
    }

    uint32 virtualAddr = (uint32)&end + KB(4);
    pageTableEntryID = (virtualAddr >> PAGE_OFFSET_BIT) & ((1 << PAGE_TABLE_BIT)-1);
    initializePageTableEntry(tableAddr, pageTableEntryID, kernelStack_phy, 1, 0);
}

void initializeTablex(PageTable tablex_vir, Page user_stack_phy) {
    initializePageTableEntry(tablex_vir, 1023, user_stack_phy, 1, 1);
}

pid32	create(
	  void		*funcaddr,	/* Address of the function	    */
	  uint32	ssize,		/* Stack size in bytes		    */
	  pri16		priority,	/* Process priority > 0		    */
	  char		*name,		/* Name (for debugging)		    */
	  uint32	nargs,		/* Number of args that follow	*/
	  ...
	)
{
	uint32		savsp, *pushsp;
	intmask 	mask;    	    /* Interrupt mask		        */
	pid32		pid;		    /* Stores new process id	    */
	struct	procent	*prptr;		/* Pointer to proc. table entry */
	int32		i;
	uint32		*a;		        /* Points to list of args	    */
	uint32		*saddr = (KERNEL_STACK_BASE - KB(4));		    /* Stack address		        */
    uint32		*saddr_user = (USER_STACK_BASE - KB(4));
    uint32      CS = 0x23;
    uint32      SS = 0x33;
    uint32      ESP;
	mask = disable();
	if (ssize < MINSTK)
		ssize = MINSTK;
	ssize = (uint32) roundmb(ssize);
	// if ( (priority < 1) || ((pid=newpid()) == SYSERR) ||
	//      ((saddr = (uint32 *)getstk(ssize)) == (uint32 *)SYSERR) || 
    //      ((saddr_user = (uint32 *)getstk(ssize)) == (uint32 *)SYSERR)) {
	// 	restore(mask);
	// 	return SYSERR;
	// }
	if ( (priority < 1) || ((pid=newpid()) == SYSERR) ) {
		restore(mask);
		return SYSERR;
	}

    kprintf("%s\n", name);
    // kprintf("%x, %x", saddr, saddr_user);
	prcount++;
	prptr = &proctab[pid];

	/* Initialize process table entry for new process */
	prptr->prstate = PR_SUSP;	/* Initial state is suspended	*/
	prptr->prprio = priority;
    prptr->prstkbase = (char *) saddr;
    prptr->prstkbase_user = (char *) saddr_user;
    prptr->prstklen = KB(4);    /*kernel stack size is always 4KB*/
	prptr->prstklen_user = ssize;
	prptr->prname[PNMLEN-1] = NULLCH;
	for (i=0 ; i<PNMLEN-1 && (prptr->prname[i]=name[i])!=NULLCH; i++)
		;
	prptr->prsem = -1;
	prptr->prparent = (pid32)getpid();
	prptr->prhasmsg = FALSE;

	/* Set up stdin, stdout, and stderr descriptors for the shell	*/
	prptr->prdesc[0] = CONSOLE;
	prptr->prdesc[1] = CONSOLE;
	prptr->prdesc[2] = CONSOLE;

    /* Allocate the space for new process's user and kernel stack */
    PageDirectory newpgdir_vir = TMP_VIRTUAL_ADDR;
    PageDirectory newpgdir_phy = allocateVirtualPage(prptr->pageDirectory, newpgdir_vir, 0);
    PageTable table0_vir = TMP_VIRTUAL_ADDR+KB(4);
    PageTable table0_phy = allocateVirtualPage(prptr->pageDirectory, table0_vir, 0);
    Page kernel_stack_vir = TMP_VIRTUAL_ADDR+KB(8);
    Page kernel_sack_phy = allocateVirtualPage(prptr->pageDirectory, kernel_stack_vir, 0);
    PageTable tablex_vir = TMP_VIRTUAL_ADDR+KB(12);
    PageTable tablex_phy = allocateVirtualPage(prptr->pageDirectory, tablex_vir, 0);
    Page user_stack_vir = TMP_VIRTUAL_ADDR+KB(16);
    Page user_stack_phy = allocateVirtualPage(prptr->pageDirectory, user_stack_vir, 0);

    initializeTable0(table0_vir, kernel_sack_phy);
    initializeTablex(tablex_vir, user_stack_phy);
    initializePageDirectoryEntry(newpgdir_vir, 0, table0_phy, 1, 1);
    initializePageDirectoryEntry(newpgdir_vir, 1, (PageDirectoryEntry *)(VIRTUAL_PAGE_DIRECTORY_ADDR + 4)->pageBaseAddress, 1, 0);
    initializePageDirectoryEntry(newpgdir_vir, 0, newpgdir_phy, 1, 1);
    initializePageDirectoryEntry(newpgdir_vir, getPageDirectoryEntryID(USER_STACK_BASE-KB(4)), newpgdir_phy, 1, 1);
	/* Initialize stack as if the process was called		*/

    saddr = (uint32 *)(TMP_VIRTUAL_ADDR + KB(12) - 4);
    saddr_user = (uint32 *)(TMP_VIRTUAL_ADDR + KB(5) - 4);
	*saddr = STACKMAGIC;
    *saddr_user = STACKMAGIC;
	savsp = (uint32)saddr;

	/* Push arguments */
	a = (uint32 *)(&nargs + 1);	/* Start of args		*/
	a += nargs -1;			/* Last argument		*/
	for ( ; nargs > 0 ; nargs--) { /* Machine dependent; copy args	*/
        uint32 tmp = *a--;
        *--saddr = tmp;/* onto created process's stack	*/
        *--saddr_user = tmp;
    }	
		
	*--saddr = (long)INITRET;	/* Push on return address	*/
    *--saddr_user = (long)INITRET;  
 
    ESP = (uint32)saddr_user;
    *--saddr = SS;
    *--saddr = ESP;
    *--saddr = 0x00000200;		/* New process runs with	*/
    *--saddr = CS;
    *--saddr = (long)funcaddr;
    *--saddr = (uint32)ret_k2u;

    //set ebp
    *--saddr = savsp;		/* This will be register ebp	*/
    savsp = (uint32) saddr;		/* Start of frame for ctxsw	*/
    
    //pushfl
    *--saddr = 0x00000200;		/* New process runs with	*/
    
    // pushal
    *--saddr = 0;			/* %eax */
    *--saddr = 0;			/* %ecx */
    *--saddr = 0;			/* %edx */
    *--saddr = 0;			/* %ebx */
    *--saddr = 0;			/* %esp; value filled in below	*/
    pushsp = saddr;			/* Remember this location	*/
    *--saddr = savsp;		/* %ebp (while finishing ctxsw)	*/
    *--saddr = 0;			/* %esi */
    *--saddr = 0;			/* %edi */
    *pushsp = (unsigned long) (prptr->prstkptr = (char *)saddr);

    saddr = (uint32 *)((uint32)saddr - KB(12));
    saddr_user = (uint32 *)((uint32)saddr_user + USER_STACK_BASE - TMP_VIRTUAL_ADDR - KB(5));
    prptr->prstkptr_user = (char *)saddr_user;
    /* clear the temporary connection between kid and  parent process*/
    for(uint32 addr = TMP_VIRTUAL_ADDR; addr < TMP_VIRTUAL_ADDR+ KB(20); addr += KB(4)) {
        clearPageTableEntry(MB(8), getPageEntryID(addr));
    }
	restore(mask);
	return pid;
}

/*------------------------------------------------------------------------
 *  newpid  -  Obtain a new (free) process ID
 *------------------------------------------------------------------------
 */
local	pid32	newpid(void)
{
	uint32	i;			/* Iterate through all processes*/
	static	pid32 nextpid = 1;	/* Position in table to try or	*/
					/*   one beyond end of table	*/

	/* Check all NPROC slots */

	for (i = 0; i < NPROC; i++) {
		nextpid %= NPROC;	/* Wrap around to beginning */
		if (proctab[nextpid].prstate == PR_FREE) {
			return nextpid++;
		} else {
			nextpid++;
		}
	}kprintf("newpid error\n");
	return (pid32) SYSERR;
}
