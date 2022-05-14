/* meminit.c - memory bounds and free list init */

#include <xinu.h>


/* Memory bounds */

void	*minheap;		/* Start of heap			*/
void	*maxheap;		/* Highest valid heap address		*/

/* Memory map structures */

uint32	bootsign = 1;		/* Boot signature of the boot loader	*/

struct	mbootinfo *bootinfo = (struct mbootinfo *)1;
				/* Base address of the multiboot info	*/
				/*  provided by GRUB, initialized just	*/
				/*  to guarantee it is in the DATA	*/
				/*  segment and not the BSS		*/

/* Segment table structures */

/* Segment Descriptor */

struct __attribute__ ((__packed__)) sd {
	unsigned short	sd_lolimit;
	unsigned short	sd_lobase;
	unsigned char	sd_midbase;
	unsigned char   sd_access;
	unsigned char	sd_hilim_fl;
	unsigned char	sd_hibase;
};

#define	NGD			8	/* Number of global descriptor entries	*/
#define FLAGS_GRANULARITY	0x80
#define FLAGS_SIZE		0x40
#define	FLAGS_SETTINGS		(FLAGS_GRANULARITY | FLAGS_SIZE)

struct sd gdt_copy[NGD] = {
/*   sd_lolimit  sd_lobase   sd_midbase  sd_access   sd_hilim_fl sd_hibase */
/* 0th entry NULL */
{            0,          0,           0,         0,            0,        0, },
/* 1st, Kernel Code Segment */
{       0xffff,          0,           0,      0x9a,         0xcf,        0, },
/* 2nd, Kernel Data Segment */
{       0xffff,          0,           0,      0x92,         0xcf,        0, },
/* 3rd, Kernel Stack Segment */
{       0xffff,          0,           0,      0x92,         0xcf,        0, },
/* 4rd, user code Segment */
{       0xffff,          0,           0,      0xfa,         0xc0,        0, },
/* 5rd, user Data Segment */
{       0xffff,          0,           0,      0xf2,         0xc0,        0, },
/* 6rd, user Stack Segment */
{       0xffff,          0,           0,      0xf2,         0xc0,        0, },
/* 7th entry NULL */
{            0,          0,           0,      0x89,            0,        0, },
};

taskstate tss_copy;
extern	struct	sd gdt[];	/* Global segment table			*/
extern taskstate tss[];
extern uint32 freePageCount;
extern void *freePages;
/*------------------------------------------------------------------------
 * meminit - initialize memory bounds and the free memory list
 *------------------------------------------------------------------------
 */
void	meminit(void) {
	struct	mbmregion	*mmap_addr;	/* Ptr to mmap entries		*/
	struct	mbmregion	*mmap_addrend;	/* Ptr to end of mmap region	*/
	uint32	next_block_length;	/* Size of next memory block	*/

    /* Initialize the freePage from end+8KB ~ end+8KB+4MB*/
    // uint32  lastFreePhysicalpage = 0;
    memset((void*)PHYSICAL_PAGE_RECORD_ADDR, 0, MB(4));

	mmap_addr = (struct mbmregion*)NULL;
	mmap_addrend = (struct mbmregion*)NULL;

	/* Initialize the memory counters */
	/*    Heap starts at the end of Xinu image */
	minheap = PHYSICAL_PAGE_RECORD_END_ADDR;
	maxheap = minheap;

	/* Check if Xinu was loaded using the multiboot specification	*/
	/*   and a memory map was included				*/
	if(bootsign != MULTIBOOT_SIGNATURE) {
		panic("could not find multiboot signature");
	}
	if(!(bootinfo->flags & MULTIBOOT_BOOTINFO_MMAP)) {
		panic("no mmap found in boot info");
	}

	/* Get base address of mmap region (passed by GRUB) */
	mmap_addr = (struct mbmregion*)bootinfo->mmap_addr;

	/* Calculate address that follows the mmap block */
	mmap_addrend = (struct mbmregion*)((uint8*)mmap_addr + bootinfo->mmap_length);

	/* Read mmap blocks and initialize the Xinu free memory list	*/
    // kprintf("end is %8x\n", (uint32)minheap);
    kprintf("free page record start at %8x\n", PHYSICAL_PAGE_RECORD_ADDR);
	while(mmap_addr < mmap_addrend) {
        kprintf("%8x ~ %8x\n", (uint32)mmap_addr->base_addr, (uint32)mmap_addr->base_addr+(uint32)mmap_addr->length);
		/* If block is not usable, skip to next block */
		if(mmap_addr->type != MULTIBOOT_MMAP_TYPE_USABLE) {
			mmap_addr = (struct mbmregion*)((uint8*)mmap_addr + mmap_addr->size + 4);
			continue;
		}

		if((uint32)maxheap < ((uint32)mmap_addr->base_addr + (uint32)mmap_addr->length)) {
			maxheap = (void*)((uint32)mmap_addr->base_addr + (uint32)mmap_addr->length);
		}

		/* Ignore memory blocks within the Xinu image */
		if((mmap_addr->base_addr + mmap_addr->length) < ((uint32)minheap)) {
			mmap_addr = (struct mbmregion*)((uint8*)mmap_addr + mmap_addr->size + 4);
			continue;
		}

		/* The block is usable, so add it to Xinu's memory list */

		/* This block straddles the end of the Xinu image */
		if((mmap_addr->base_addr <= (uint32)minheap) &&
		  ((mmap_addr->base_addr + mmap_addr->length) >
		  (uint32)minheap)) {
            appendFreePhysicalPage(
                /* This is the first free block, base address is the minheap */
                roundmb((uint32)minheap), 
                /* Subtract Xinu image from length of block */
                truncmb(mmap_addr->base_addr + mmap_addr->length - (uint32)minheap));
		} else {
            appendFreePhysicalPage(
                /* Handle a free memory block other than the first one */
                roundmb(mmap_addr->base_addr), 
                /* Initialize the length of the block */
                truncmb(mmap_addr->length));
		}

		/* Move to the next mmap block */
		mmap_addr = (struct mbmregion*)((uint8*)mmap_addr + mmap_addr->size + 4);
	}

    // kprintf("freePageCount is %d\n", freePageCount);
    // uint8 t = 0;
    // for(int cnt = 0; cnt < freePageCount; cnt++) {
    //     t = (++t) % 10; 
    //     if(!t)
    //         kprintf("\n");
    //     kprintf("%8x\t", *(uint32 *)((uint32)freePages + 4*cnt));
    // }
}

// static inline void ltr(uint16 sel) {
//   asm volatile("ltr %0" : : "r" (sel));
// }

/*------------------------------------------------------------------------
 * setsegs  -  Initialize the global segment table
 *------------------------------------------------------------------------
 */
void	setsegs()
{
	extern int	etext;
	struct sd	*psd;
	uint32		np, ds_end;

	ds_end = 0xffffffff/PAGE_SIZE; /* End page number of Data segment */

	psd = &gdt_copy[1];	/* Kernel code segment: identity map from address
				   0 to etext */
	np = ((int)&etext - 0 + PAGE_SIZE-1) / PAGE_SIZE;	/* Number of code pages */
	psd->sd_lolimit = np;
	psd->sd_hilim_fl = FLAGS_SETTINGS | ((np >> 16) & 0xff);

	psd = &gdt_copy[2];	/* Kernel data segment */
	psd->sd_lolimit = ds_end;
	psd->sd_hilim_fl = FLAGS_SETTINGS | ((ds_end >> 16) & 0xff);

	psd = &gdt_copy[3];	/* Kernel stack segment */
	psd->sd_lolimit = ds_end;
	psd->sd_hilim_fl = FLAGS_SETTINGS | ((ds_end >> 16) & 0xff);

    psd = &gdt_copy[7];	/* task statu segment */
    psd->sd_lolimit = 0xffff & (0x68-1);
    psd->sd_lobase = ((long)tss) & 0xffff;
    psd->sd_midbase = (((long)tss) & 0xff0000) >> 16;
    psd->sd_hibase = (((long)tss) & 0xff000000) >> 24;
    // kprintf("%x %02x %02x %04x\n", &tss_copy, psd->sd_hibase, psd->sd_midbase, psd->sd_lobase);
    tss_copy.ss0 = (0x3 << 3);
    tss_copy.iomb = (uint16) 0xffff;
    // ltss();
    
	memcpy(gdt, gdt_copy, sizeof(gdt_copy));
    memcpy(tss, &tss_copy, sizeof(tss_copy));
}
