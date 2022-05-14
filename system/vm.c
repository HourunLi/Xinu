#include<xinu.h>
uint32 lastFreePhysicalPageRecordAddress; // Last free physical page addr
uint32 kpgdir;
uint32 freePageCount = 0;
void *freePages = PHYSICAL_PAGE_RECORD_ADDR;

// void recordFreePhysicalPage(uint32 addr, uint32 size) {
//     uint32 eaddr = addr + size;
//     for(uint32 i = lastFreePhysicalPageRecordAddress + 4; i < RecordAddress(roundpage(addr)); i += 4) {
//         uint32 *recordAddr = (uint32 *)i;
//         *recordAddr = lastFreePhysicalPageRecordAddress;
//     }
//     for(uint32 i = roundpage(addr); i < eaddr; i += VM_PAGE_SIZE) {
//         uint32 *recordAddr = (uint32 *)RecordAddress(i);
//         *recordAddr = lastFreePhysicalPageRecordAddress;
//         lastFreePhysicalPageRecordAddress = recordAddr;
//     }
//     return;
// }

void appendFreePhysicalPage(uint32 addr, uint32 size) {
    uint32 eaddr = addr + size;
    for(uint32 phy = roundpage(addr); phy < eaddr; phy += VM_PAGE_SIZE) {
        uint32 *RecordAddr =  freePages + (freePageCount++)*4;
        *RecordAddr = phy;
        kprintf("%dth record %8x at %8x\n", freePageCount, phy, RecordAddr);
    }
}

void* allocatePhysicalPage() {
    if (freePageCount == 0) return NULL;
    void *physicalPage = *(uint32 *)((uint32)freePages + (--freePageCount)*4);
    memset(physicalPage, 0, KB(4));
    return physicalPage;
}

void freePhysicalPage(uint32 physicalAddr) {
    for (uint32 i = 0; i < freePageCount; i++)
        if (*(uint32 *)((uint32)freePages + 4*(i)) == physicalAddr) {
            kprintf("freePhysicalPage: double free or corruption (%8x)\n", physicalAddr);
            return;
        }
    *(uint32 *)((uint32)freePages + (freePageCount++)*4) = physicalAddr;
    return;
}

void initializePageDirectoryEntry(PageDirectory pageDirectory, uint32 entryID, uint32 physicalAddr, uint8 present, uint8 userSupervisor) {
    PageDirectoryEntry *entryPtr    = (PageDirectoryEntry *)((uint32)pageDirectory + entryID * PAGE_ENTRY_SIZE);
    entryPtr->pageTableBaseAddress  = physicalAddr;
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
    entryPtr->pageBaseAddress   = physicalAddr;
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
/* Return the physical address of kernel page directory*/
PageDirectory initialKernelPageTable() {
    PageDirectory kernelPageDirectory = (PageDirectory)allocatePhysicalPage();

    /* Initialize the 0th page direcoty entry*/
    PageTable pageTable_0 = (PageTable)allocatePhysicalPage();
    for(uint32 virtualAddr = 0, physicalAddr = 0; physicalAddr < ((uint32)&end + KB(8)); virtualAddr += KB(4), physicalAddr += KB(4)) {
        uint32 pageTableEntryID = (virtualAddr >> PAGE_OFFSET_BIT) & ((1 << PAGE_TABLE_BIT)-1);
        uint32 userSupervisor = 0, present = 1;
        if(MB(1) <= physicalAddr && physicalAddr < (uint32)&end) {
            userSupervisor = 1;
        }

        if( (uint32)&end <= physicalAddr && physicalAddr < ((uint32)&end + KB(4)) ) {
            present = 0;
        }
        initializePageTableEntry(pageTable_0, pageTableEntryID, physicalAddr, present, userSupervisor);
    }
    /* 
     * Because we need access code and data in user mode
     * The 0th page directory must be user-accessble
     * So the userSupervisor must be 1   
     */
    initializePageDirectoryEntry(kernelPageDirectory, 0, pageTable_0, 1, 1);

    /* Initialize the 1th page direcoty entry*/
    PageTable pageTable_1 = (PageTable)allocatePhysicalPage();
    for(uint32 virtualAddr = MB(4), physicalAddr = ((uint32)&end + KB(8)); physicalAddr < ((uint32)&end + KB(8) + MB(8)); 
            virtualAddr += KB(4), physicalAddr += KB(4)) {
        uint32 pageTableEntryID = (virtualAddr >> PAGE_OFFSET_BIT) & ((1 << PAGE_TABLE_BIT)-1);
        initializePageTableEntry(pageTable_1, pageTableEntryID, physicalAddr, 1, 0);
    }
    /* Free physical page must be kernel accessble? */
    initializePageDirectoryEntry(kernelPageDirectory, 1, pageTable_1, 1, 0);

    /* Initialize the 2th page direcoty entry -- the page directory itself(self to self mapping)*/
    initializePageDirectoryEntry(kernelPageDirectory, 2, kernelPageDirectory, 1, 1);

    return kernelPageDirectory;
}

bool8   enablePaging(PageDirectory pageDirectoryPhyAddr) {
    loadCr3(pageDirectoryPhyAddr);
    setCr0();
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
        "mov    %%cr0, %%eax;       \
        or      %0, %%eax;          \
        mov     %%eax, %%cr0;"
        :
        : "m"(flag)
        : "eax"
    );
    return;
}