#include<xinu.h>
uint32 freePageCount = 0;
void *freePages = PHY_PAGE_RECORD_ADDR;

void appendFreePhysicalPage(uint32 addr, uint32 size) {
    /* It is in initialize stage and paging is not enabled */
    uint32 eaddr = addr + size;
    for(uint32 phy = roundpage(addr); phy < eaddr; phy += VM_PAGE_SIZE) {
        uint32 *RecordAddr =  (uint32 *)((uint32)freePages + (freePageCount++)*4);
        *RecordAddr = phy;
        // kprintf("%dth record %8x at %8x\n", freePageCount, phy, RecordAddr);
    }
}

void* allocatePhysicalPage() {
    if (freePageCount == 0) {
        kprintf("There is not free physical pages\n");
        return NULL;
    }
    void *physicalPage = *(uint32 *)((uint32)freePages + (--freePageCount)*4);
    // kpritnf("Allocate physical page %8x\n", physicalPage);
    return physicalPage;
}

void freePhysicalPage(uint32 physicalAddr) {
    for (uint32 i = 0; i < freePageCount; i++)
        if (*(uint32 *)((uint32)freePages + 4*(i)) == physicalAddr) {
            kprintf("freePhysicalPage: double free or corruption (%8x)\n", physicalAddr);
            return;
        }
    kprintf("free physical page %8x\n", physicalAddr);
    *(uint32 *)((uint32)freePages + (freePageCount++)*4) = physicalAddr;
    return;
}

/* Only for heap allocate */
void allocateVirtualAddr(PageDirectory pageDirectory, uint32 virtualAddr, uint32 size, uint8 userSupervisor) {
    kprintf("allocateVirtualAddr: Alloc virtual addr %8x with %x bytes\n", virtualAddr, size);
    uint32 endVirtualAddr = virtualAddr+size;

    for(uint32 addr = virtualAddr; addr < endVirtualAddr; addr += KB(4)) {
        allocateVirtualPage(pageDirectory, addr, userSupervisor);
    }
}

void *allocateVirtualPage(PageDirectory pageDirectory, uint32 virtualAddr, uint8 userSupervisor) {
    kprintf("allocateVirtualPage: Alloc virtual addr %8x\n", virtualAddr);
    uint32 pgdirEntryID = getPageDirectoryEntryID(virtualAddr);
    uint32 pgEntryID = getPageEntryID(virtualAddr);
    uint32 pgOffset  = getPageOffset(virtualAddr);
    PageTable pageTable;
    PageDirectoryEntry *pgdirEntryPtr;
    PageTableEntry *pgEntryPtr, *pageEntry;

    /* Get the virtual addr of page directory if paging enabled*/
    if(getPagingEnabled()) {
        pageDirectory = VM_PAGE_DIRECTORY_ADDR;
    }
    /* If the entry in page directory or page is not existent, then allocate */
    pgdirEntryPtr = (PageDirectoryEntry *)((uint32)pageDirectory + pgdirEntryID * PAGE_ENTRY_SIZE);
    /* entry in page directory is null */
    if(pgdirEntryPtr->present == 0) {
        pageTable = (PageTable)allocatePhysicalPage();
        initializePageDirectoryEntry(pageDirectory, pgdirEntryID, pageTable, 1, userSupervisor);
        memset(VM_PAGE_TABLE0 + pgdirEntryID * VM_PAGE_SIZE, 0, KB(4));
    }

    /* Get the virtual addr of page table if paging enabled*/
    if(getPagingEnabled()) {
        pageTable = VM_PAGE_TABLE0 + pgdirEntryID * VM_PAGE_SIZE;
    }
    pgEntryPtr = (PageTableEntry *)((uint32)pageTable + pgEntryID * PAGE_ENTRY_SIZE);
    if(pgEntryPtr->present == 0) {
        pageEntry = (PageTableEntry *)allocatePhysicalPage();
        initializePageTableEntry(pageTable, pgEntryID, pageEntry, 1, userSupervisor);
        invlpg(truncpage(virtualAddr));
        memset(truncpage(virtualAddr), 0, KB(4));
        // return pageEntry;
    }
    return (pgEntryPtr->pageBaseAddress) << PAGE_OFFSET_BIT;
}

/* Only for heap free */
void freeVirtualAddr(PageDirectory pageDirectory, uint32 virtualAddr, uint32 size) {
    kprintf("freeVirtualAddr: free virtual addr %8x with %x bytes\n", virtualAddr, size);
    uint32 endVirtualAddr = virtualAddr+size;
    for(uint32 addr = roundpage(virtualAddr); addr < endVirtualAddr; addr += KB(4)) {
        memset(addr, 0, KB(4));
        freeVirtualPage(pageDirectory, addr);
        invlpg(addr);
    }
    return;
}

void freeVirtualPage(PageDirectory pageDirectory, uint32 virtualAddr) {
    uint32 pgdirEntryID = getPageDirectoryEntryID(virtualAddr);
    uint32 pgEntryID = getPageEntryID(virtualAddr);
    uint32 pgOffset  = getPageOffset(virtualAddr);
    PageTable pageTable;
    PageDirectoryEntry *pgdirEntryPtr;
    PageTableEntry *pgEntryPtr, *pageEntry;

    /* Get the virtual addr of page directory if paging enabled*/
    if(getPagingEnabled()) {
        pageDirectory = VM_PAGE_DIRECTORY_ADDR;
    }

    /* If the entry in page directory or page is not existent, then allocate */
    pgdirEntryPtr = (PageDirectoryEntry *)((uint32)pageDirectory + pgdirEntryID * PAGE_ENTRY_SIZE);
    /* entry in page directory is null */
    if(pgdirEntryPtr->present == 0) {
        kprintf("Error: Can't find page directory entry in page directory\n");
        return;
    }

    /* Get the virtual addr of page table if paging enabled*/
    if(getPagingEnabled()) {
        pageTable = MB(8) + pgdirEntryID * KB(4);
    } else{
        pageTable = (pgdirEntryPtr->pageTableBaseAddress) << PAGE_OFFSET_BIT;
    }

    pgEntryPtr = (PageTableEntry *)((uint32)pageTable + pgEntryID * PAGE_ENTRY_SIZE);
    if(pgEntryPtr->present == 0) {
        kprintf("Error: Can't find page entry in page table\n");    
        return;
    }
    freePhysicalPage((pgEntryPtr->pageBaseAddress) << PAGE_OFFSET_BIT);
    memset(pgEntryPtr, 0, B(4));
    return;
}


void initializePageDirectoryEntry(PageDirectory pageDirectory, uint32 entryID, uint32 physicalAddr, uint8 present, uint8 userSupervisor) {
    PageDirectoryEntry *entryPtr    = (PageDirectoryEntry *)((uint32)pageDirectory + entryID * PAGE_ENTRY_SIZE);
    entryPtr->pageTableBaseAddress  = (physicalAddr >> PAGE_OFFSET_BIT);
    entryPtr->avail                 = 0;
    entryPtr->globalPage            = 0;
    entryPtr->pageSize              = 0;
    entryPtr->reserved              = 0;
    entryPtr->accessed              = 0;
    entryPtr->cacheDisabled         = 0;
    entryPtr->writeThrough          = 1;
    entryPtr->userSupervisor        = userSupervisor;
    entryPtr->readOrWrite           = 1;
    entryPtr->present               = present;
    return;
}

void initializePageTableEntry(PageTable pageTable, uint32 entryID, uint32 physicalAddr, uint8 present, uint8 userSupervisor) {
    PageTableEntry *entryPtr    = (PageTableEntry *)((uint32)pageTable + entryID * PAGE_ENTRY_SIZE);
    entryPtr->pageBaseAddress   = (physicalAddr >> PAGE_OFFSET_BIT);
    entryPtr->avail             = 0;
    entryPtr->globalPage        = 0;
    entryPtr->reserved          = 0;
    entryPtr->dirty             = 0;
    entryPtr->accessed          = 0; 
    entryPtr->cacheDisabled     = 0;
    entryPtr->writeThrough      = 1;
    entryPtr->userSupervisor    = userSupervisor;
    entryPtr->readOrWrite       = 1;
    entryPtr->present           = present;
    return;
}

void clearPageTableEntry(PageTable pageTable, uint32 entryID) {
    PageTableEntry *entryPtr = (PageTableEntry *)((uint32)pageTable + entryID * PAGE_ENTRY_SIZE);
    entryPtr->pageBaseAddress   = 0;
    entryPtr->avail             = 0;
    entryPtr->globalPage        = 0;
    entryPtr->reserved          = 0;
    entryPtr->dirty             = 0;
    entryPtr->accessed          = 0; 
    entryPtr->cacheDisabled     = 0;
    entryPtr->writeThrough      = 0;
    entryPtr->userSupervisor    = 0;
    entryPtr->readOrWrite       = 0;
    entryPtr->present           = 0;
    return;
}

/* Return the physical address of kernel page directory*/
PageDirectory initialKernelPageTable() {
    /* kernel page directory page 4KB */
    PageDirectory kernelPageDirectory = (PageDirectory)allocatePhysicalPage();

    /* Initialize the 0th page */
    PageTable pageTable_0 = (PageTable)allocatePhysicalPage();
    for(uint32 virtualAddr = 0, physicalAddr = 0; physicalAddr < PHY_PAGE_RECORD_ADDR; virtualAddr += KB(4), physicalAddr += KB(4)) {
        uint32 pageTableEntryID = getPageEntryID(virtualAddr);
        uint32 userSupervisor = 0, present = 1;
        if(MB(1) <= physicalAddr && physicalAddr < (uint32)&end) {
            userSupervisor = 1;
        }

        if( (uint32)&end <= physicalAddr && physicalAddr < ((uint32)&end + KB(4)) ) {
            present = 0;
        }
        /* Initialize the first page */
        initializePageTableEntry(pageTable_0, pageTableEntryID, physicalAddr, present, userSupervisor);
    }
    /* 
     * Initialize the 0th page directory entry
     * Because we need access code and data in user mode
     * The 0th page directory must be user-accessble
     * So the userSupervisor must be 1   
     */
    initializePageDirectoryEntry(kernelPageDirectory, 0, pageTable_0, 1, 1);

    /* Initialize the 1th page */
    PageTable pageTable_1 = (PageTable)allocatePhysicalPage();
    for(uint32 virtualAddr = MB(4), physicalAddr = PHY_PAGE_RECORD_ADDR; physicalAddr < PHY_PAGE_RECORD_END_ADDR; 
            virtualAddr += KB(4), physicalAddr += KB(4)) {
        uint32 pageTableEntryID = getPageEntryID(virtualAddr);
        initializePageTableEntry(pageTable_1, pageTableEntryID, physicalAddr, 1, 0);
    }
    /* 
     * Initialize the 1th page directory entry
     * Free physical page must be kernel accessble? 
     */
    initializePageDirectoryEntry(kernelPageDirectory, 1, pageTable_1, 1, 0);

    /* Initialize the 2th page direcoty entry -- the page directory itself(self to self mapping)*/
    initializePageDirectoryEntry(kernelPageDirectory, 2, kernelPageDirectory, 1, 1);

    return kernelPageDirectory;
}


void enablePaging(PageDirectory pageDirectoryPhyAddr) {
    kprintf("Attention: enable paging!\n");
    loadCr3(pageDirectoryPhyAddr);
    setCr0();
    freePages = VM_PAGE_RECORD_ADDR;
}

void loadCr3(PageDirectory pageDirectoryPhyAddr) {
    asm volatile (
        "mov %0, %%cr3;"
        :
        :"r"(pageDirectoryPhyAddr)
        :
    );
    return;
}

void setCr0() {
    // Enable paging
    uint32 flag = CR0_PG | CR0_WP;
    asm volatile (
        "movl    %%cr0, %%eax;       \
        orl      %0, %%eax;          \
        movl     %%eax, %%cr0;"
        :
        : "m"(flag)
        : "eax"
    );
    return;
}

bool8 getPagingEnabled() {
    uint32 cr0;
    asm volatile (
        "movl %%cr0, %0;"
        :"=r"(cr0)
        :
        :
    );
    return (cr0 & CR0_PG) != 0;
}

