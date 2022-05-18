#ifndef __KERNEL_H__
#define __KERNEL_H__
/* kernel.h */

/* General type declarations used throughout the kernel */

typedef	unsigned char	byte;
typedef	unsigned char	uint8;
typedef	int		int32;
typedef	short		int16;
typedef	unsigned int	uint32;
typedef	unsigned short	uint16;
typedef	unsigned long long uint64;

/* Xinu-specific types */

typedef	int32	sid32;		/* semaphore ID				*/
typedef	int16	qid16;		/* queue ID				*/
typedef	int32	pid32;		/* process ID				*/
typedef	int32	did32;		/* device ID				*/
typedef	int16	pri16;		/* process priority			*/
typedef	uint32	umsg32;		/* message passed among processes	*/
typedef	int32	bpid32;		/* buffer pool ID			*/
typedef	byte	bool8;		/* Boolean type				*/
typedef	uint32	intmask;	/* saved interrupt mask			*/
typedef	int32	ibid32;		/* index block ID (used in file system)	*/
typedef	int32	dbid32;		/* data block ID (used in file system)	*/
typedef	int32	uid32;		/* ID for UDP table descriptor		*/

/* Function declaration return types */

typedef int32	syscall;	/* system call declaration		*/
typedef int32	devcall;	/* device call declaration		*/
typedef int32	shellcmd;	/* shell command declaration		*/
typedef int32	process;	/* top-level function of a process	*/
typedef	void	interrupt;	/* interrupt procedure			*/
typedef	int32	status;		/* returned status value (OK/SYSERR)	*/

#define local	static		/* Local procedure or variable declar.	*/

/* Boolean constants */

#define FALSE	0		/* Boolean False			*/
#define TRUE	1		/* Boolean True				*/

/* Null pointer, character, and string definintions */

#define NULL	0		/* null pointer for linked lists	*/
#define NULLCH	'\0'		/* null character			*/
#define	NULLSTR	""		/* null string				*/

/* Universal return constants */

#define OK	( 1)		/* normal system call return		*/
#define	SYSERR	(-1)		/* system call failed			*/
#define	EOF	(-2)		/* End-of-file (usually from read)	*/
#define	TIMEOUT	(-3)		/* system call timed out		*/

extern	qid16	readylist;	/* global ID for list of ready processes*/

#define	MINSTK	KB(4)		/* minimum stack size in bytes		*/
#define MAXSTK  MB(4)

#define	CONTEXT	64		/* bytes in a function call context on	*/
				/* the run-time stack			*/
#define	QUANTUM  20		/* time slice in milliseconds		*/

/* Size of the stack for the null process */

#define	NULLSTK		8192	/* stack size for null process		*/

/* Prototypes of I/O functions used throughout the kernel */

syscall	kprintf(char *fmt, ...);
syscall	kputc(byte);
syscall	kgetc(void);

typedef struct TASKSTATE {
    unsigned int link;         // Old ts selector
    unsigned int esp0;         // Stack pointers and segment selectors
    unsigned short ss0;        //   after an increase in privilege level
    unsigned short padding1;
    unsigned int *esp1;
    unsigned short ss1;
    unsigned short padding2;
    unsigned int *esp2;
    unsigned short ss2;
    unsigned short padding3;
    void *cr3;         // Page directory base
    unsigned int *eip;         // Saved state from last task switch
    unsigned int eflags;
    unsigned int eax;          // More saved state (registers)
    unsigned int ecx;
    unsigned int edx;
    unsigned int ebx;
    unsigned int *esp;
    unsigned int *ebp;
    unsigned int esi;
    unsigned int edi;
    unsigned short es;         // Even more saved state (segment selectors)
    unsigned short padding4;
    unsigned short cs;
    unsigned short padding5;
    unsigned short ss;
    unsigned short padding6;
    unsigned short ds;
    unsigned short padding7;
    unsigned short fs;
    unsigned short padding8;
    unsigned short gs;
    unsigned short padding9;
    unsigned short ldt;
    unsigned short padding10;
    unsigned short t;          // Trap on task switch
    unsigned short iomb;       // I/O map base address
}taskstate;

#endif