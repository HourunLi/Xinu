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
        pageTableEntryID = getPageEntryID(virtualAddr);
        uint32 userSupervisor = 0;
        if(MB(1) <= physicalAddr && physicalAddr < (uint32)&end) {
            userSupervisor = 1;
        }
        /* Initialize the first page */
        initializePageTableEntry(tableAddr, pageTableEntryID, physicalAddr, 1, 1);
    }

    virtualAddr = KERNEL_STACK_BASE - KB(4);
    initializePageTableEntry(tableAddr, getPageEntryID(virtualAddr), kernelStack_phy, 1, 1);
}

void initializeTablex(PageTable tablex_vir, uint32 virtualAddr, Page user_stack_phy) {
    initializePageTableEntry(tablex_vir, getPageEntryID(virtualAddr), user_stack_phy, 1, 1);
}

/* adapt the temporary kernel stack pointer(esp) to normal kernel stack mode*/
uint32 *kernelStackAdptor_tmp2normal(uint32 *esp) {
    return (uint32 *)((uint32)esp + KERNEL_STACK_BASE - TMP_VIRTUAL_ADDR - 3 * VM_PAGE_SIZE);
}
/* adapt the temporary user stack pointer(esp) to normal user stack mode*/
uint32 *userStackAdaptor_tmp2normal(uint32 *esp) {
    return (uint32 *)((uint32)esp + USER_STACK_BASE - TMP_VIRTUAL_ADDR - 5 * VM_PAGE_SIZE);
}

/* adapt the temporary user stack pointer in last page(esp) to normal user stack mode*/
uint32 *userStackAdaptor_lastPage_tmp2normal(uint32 *esp, uint32 ssize) {
    return (uint32 *)((uint32)esp - TMP_VIRTUAL_ADDR - 5 * VM_PAGE_SIZE + truncpage(USER_STACK_BASE - ssize));
    // return (uint32 *)( TMP_VIRTUAL_ADDR + 6 * VM_PAGE_SIZE - (roundpage(esp) - (uint32)esp) );
}

/* adapt the normal kernel stack pointer(esp) to normal kernel stack mode*/
uint32 *kernelStackAdptor_normal2tmp(uint32 *esp) {
    return (uint32 *)((uint32)esp + TMP_VIRTUAL_ADDR + 3 * VM_PAGE_SIZE - KERNEL_STACK_BASE);
}

/* adapt the normal user stack pointer(esp) to temporary user stack mode*/
uint32 *userStackAdaptor_normal2tmp(uint32 *esp) {
    return (uint32 *)((uint32)esp + TMP_VIRTUAL_ADDR + 5 * VM_PAGE_SIZE - USER_STACK_BASE);
}

/* adapt the normal user stack pointer in last page(esp) to temporary user stack mode*/
uint32 *userStackAdaptor_lastPage_normal2tmp(uint32 *esp) {
    // return (uint32 *)((uint32)esp + TMP_VIRTUAL_ADDR + 5 * VM_PAGE_SIZE - USER_STACK_BASE);
    return (uint32 *)( TMP_VIRTUAL_ADDR + 5 * VM_PAGE_SIZE + ((uint32)esp - truncpage(esp)) );
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
	intmask 	mask;    	    /* Interrupt mask		                */
	pid32		pid;		    /* Stores new process id	            */
	struct	procent	*prptr;		/* Pointer to proc. table entry         */
	int32		i;
	uint32		*a;		        /* Points to list of args	            */
	uint32		*saddr; /* Stack address  */
    uint32		*saddr_user;
    uint32      CS = 0x23;
    uint32      SS = 0x33;
    uint32      ESP;
	mask = disable();

	if (ssize < MINSTK)
		ssize = MINSTK;
    if(ssize > MAXSTK)
        ssize = MAXSTK;

	ssize = (uint32) roundpage(ssize);
	if ( (priority < 1) || ((pid=newpid()) == SYSERR)) {
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
    prptr->prstkbase = (char *)(KERNEL_STACK_BASE - KB(4));
    prptr->prstkbase_user = (char *)(USER_STACK_BASE - KB(4));
    prptr->prstklen = KB(4);        /*kernel stack size is always 4KB*/
	prptr->prstklen_user = ssize;   /*USER stack size is round to 4KB*n */
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

    /* clear the temporary connection between kid and  parent process*/
    for(uint32 addr = TMP_VIRTUAL_ADDR; addr < TMP_VIRTUAL_ADDR + 6 * VM_PAGE_SIZE; addr += VM_PAGE_SIZE) {
        clearPageTableEntry(MB(8), getPageEntryID(addr));
    }

    /* Allocate the space for new process's user and kernel stack */
    PageDirectory newpgdir_vir = TMP_VIRTUAL_ADDR;  // end + 8KB
    void *newpgdir_phy = allocateVirtualPage(VIRTUAL_PAGE_DIRECTORY_ADDR, newpgdir_vir, 1);
    PageTable table0_vir = TMP_VIRTUAL_ADDR + 1 * VM_PAGE_SIZE;  // end + 12KB
    void *table0_phy = allocateVirtualPage(VIRTUAL_PAGE_DIRECTORY_ADDR, table0_vir, 1);
    Page kernel_stack_vir = TMP_VIRTUAL_ADDR + 2 * VM_PAGE_SIZE; // end + 16KB
    void *kernel_stack_phy = allocateVirtualPage(VIRTUAL_PAGE_DIRECTORY_ADDR, kernel_stack_vir, 1);
    PageTable tablex_vir = TMP_VIRTUAL_ADDR + 3 * VM_PAGE_SIZE; // end + 20KB
    void *tablex_phy = allocateVirtualPage(VIRTUAL_PAGE_DIRECTORY_ADDR, tablex_vir, 1);

    /* initialize page table 0 (0KB - 4KB)*/
    initializeTable0(table0_vir, kernel_stack_phy);
    /* initialize page directory 0th entry*/
    initializePageDirectoryEntry(newpgdir_vir, 0, table0_phy, 1, 1);
    /* 
     * initialize child process's page directory 1th entry
     * By directly using parent's page directory 1th entry
     */
    initializePageDirectoryEntry(newpgdir_vir, 1, ((((PageDirectoryEntry *)(VIRTUAL_PAGE_DIRECTORY_ADDR + B(4)))->pageTableBaseAddress) << PAGE_OFFSET_BIT), 1, 1);
    initializePageDirectoryEntry(newpgdir_vir, 2, newpgdir_phy, 1, 1);

    /* Initialize user stack(size is ssize) */
    Page user_stack_vir = TMP_VIRTUAL_ADDR + 4 * VM_PAGE_SIZE; // end + 24KB
    for(uint32 addr = USER_STACK_BASE - ssize; addr < USER_STACK_BASE; addr += KB(4)) {
        clearPageTableEntry(MB(8), getPageEntryID(user_stack_vir));
        void *user_stack_phy = allocateVirtualPage(VIRTUAL_PAGE_DIRECTORY_ADDR, user_stack_vir, 1);
        initializeTablex(tablex_vir, addr, user_stack_phy);
        if(addr == USER_STACK_BASE - ssize) {
            /* initialize the last page table (end + 28KB)*/
            Page user_stack_lastpage_vir = TMP_VIRTUAL_ADDR + 5 * VM_PAGE_SIZE;
            clearPageTableEntry(MB(8), getPageEntryID(user_stack_lastpage_vir));
            initializePageTableEntry(MB(8), getPageEntryID(user_stack_lastpage_vir), user_stack_phy, 1, 1);
        }
    }
    
    initializePageDirectoryEntry(newpgdir_vir, getPageDirectoryEntryID(USER_STACK_BASE-KB(4)), tablex_phy, 1, 1);
	/* Initialize stack as if the process was called		*/
    
    saddr = (uint32 *)(TMP_VIRTUAL_ADDR + 3 * VM_PAGE_SIZE - B(4));
    saddr_user = (uint32 *)(TMP_VIRTUAL_ADDR + 5 * VM_PAGE_SIZE - B(4));
	*saddr = STACKMAGIC;
    *saddr_user = STACKMAGIC;
	// savsp = (uint32)saddr;
    savsp = (uint32)kernelStackAdptor_tmp2normal(saddr);

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
 
    // ESP = (uint32)saddr_user;
    ESP = (uint32)userStackAdaptor_tmp2normal(saddr_user);
    *--saddr = SS;
    *--saddr = ESP;
    *--saddr = 0x00000200;		/* New process runs with	*/
    *--saddr = CS;
    *--saddr = (long)funcaddr;
    *--saddr = (uint32)ret_k2u;

    //set ebp
    *--saddr = savsp;		/* This will be register ebp	*/
    // savsp = (uint32) saddr;		/* Start of frame for ctxsw	*/
    savsp = (uint32)kernelStackAdptor_tmp2normal(saddr);		/* Start of frame for ctxsw	*/


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
    // *pushsp = (unsigned long) (prptr->prstkptr = (char *)saddr);
    *pushsp = (unsigned long) (prptr->prstkptr = (char *)kernelStackAdptor_tmp2normal(saddr));
    // saddr = (uint32 *)((uint32)saddr + KERNEL_STACK_BASE - TMP_VIRTUAL_ADDR - 3 * VM_PAGE_SIZE);
    // saddr_user = (uint32 *)((uint32)saddr_user + USER_STACK_BASE - TMP_VIRTUAL_ADDR - 5 * VM_PAGE_SIZE);
    prptr->prstkptr = (char *)kernelStackAdptor_tmp2normal(saddr);	
    prptr->prstkptr_user = (char *)userStackAdaptor_tmp2normal(saddr_user);
    prptr->pageDirectory = newpgdir_phy;
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
