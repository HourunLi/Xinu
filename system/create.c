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
pid32	create(
	  void		*funcaddr,	/* Address of the function	*/
	  uint32	ssize,		/* Stack size in bytes		*/
	  pri16		priority,	/* Process priority > 0		*/
	  char		*name,		/* Name (for debugging)		*/
	  uint32	nargs,		/* Number of args that follow	*/
	  ...
	)
{
	uint32		savsp, *pushsp;
	intmask 	mask;    	/* Interrupt mask		*/
	pid32		pid;		/* Stores new process id	*/
	struct	procent	*prptr;		/* Pointer to proc. table entry */
	int32		i;
	uint32		*a;		/* Points to list of args	*/
	uint32		*saddr;		/* Stack address		*/
    uint32		*saddr_user;
    uint32      CS = 0x23;
    uint32      SS = 0x33;
    uint32      ESP;
	mask = disable();
	if (ssize < MINSTK)
		ssize = MINSTK;
	ssize = (uint32) roundmb(ssize);
	if ( (priority < 1) || ((pid=newpid()) == SYSERR) ||
	     ((saddr = (uint32 *)getstk(ssize)) == (uint32 *)SYSERR) || 
         ((saddr_user = (uint32 *)getstk(ssize)) == (uint32 *)SYSERR)) {
		restore(mask);
		return SYSERR;
	}
    // kprintf("%s\n", name);
    // kprintf("%x, %x", saddr, saddr_user);
	prcount++;
	prptr = &proctab[pid];

	/* Initialize process table entry for new process */
	prptr->prstate = PR_SUSP;	/* Initial state is suspended	*/
	prptr->prprio = priority;
	prptr->prstkbase = (char *)saddr;
    prptr->prstkbase_user = (char *)saddr_user;
	prptr->prstklen = ssize;
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

	/* Initialize stack as if the process was called		*/

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
    prptr->prstkptr_ = saddr;
    // if(strncmp(name, "Main process", 12) == 0) {
        // tss->ss0 = (0x3 << 3);
        // tss->esp0 = (long) saddr;
        // tss->ds = (0x3 << 3);
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
    // }else{
    //     /* The following entries on the stack must match what ctxsw	*/
    //     /*   expects a saved process state to contain: ret address,	*/
    //     /*   ebp, interrupt mask, flags, registers, and an old SP	*/

    //     *--saddr = (long)funcaddr;	/* Make the stack look like it's*/
    //                     /*   half-way through a call to	*/
    //                     /*   ctxsw that "returns" to the*/
    //                     /*   new process		*/
    //     *--saddr = savsp;		/* This will be register ebp	*/
    //                     /*   for process exit		*/
    //     savsp = (uint32) saddr;		/* Start of frame for ctxsw	*/
    //     *--saddr = 0x00000200;		/* New process runs with	*/
    //                     /*   interrupts enabled		*/

    //     /* Basically, the following emulates an x86 "pushal" instruction*/

    //     *--saddr = 0;			/* %eax */
    //     *--saddr = 0;			/* %ecx */
    //     *--saddr = 0;			/* %edx */
    //     *--saddr = 0;			/* %ebx */
    //     *--saddr = 0;			/* %esp; value filled in below	*/
    //     pushsp = saddr;			/* Remember this location	*/
    //     *--saddr = savsp;		/* %ebp (while finishing ctxsw)	*/
    //     *--saddr = 0;			/* %esi */
    //     *--saddr = 0;			/* %edi */
    //     *pushsp = (unsigned long) (prptr->prstkptr = (char *)saddr);
    // }
    prptr->prstkptr_user = (char *)saddr_user;
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
