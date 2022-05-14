#include<xinu.h>
uint32 lastFreePhysicalPageRecordAddress; // Last free physical page addr

void recordFreePhysicalPage(uint32 addr, uint32 size) {
    uint32 eaddr = addr + size;
    for(uint32 i = lastFreePhysicalPageRecordAddress + 4; i < RecordAddress(roundpage(addr)); i += 4) {
        uint32 *recordAddr = (uint32 *)i;
        *recordAddr = lastFreePhysicalPageRecordAddress;
    }
    for(uint32 i = roundpage(addr); i < eaddr; i += VM_PAGE_SIZE) {
        uint32 *recordAddr = (uint32 *)RecordAddress(i);
        *recordAddr = lastFreePhysicalPageRecordAddress;
        lastFreePhysicalPageRecordAddress = recordAddr;
    }
    return;
}

uint32 freePageCount = 0;
void *freePages = PHYSICAL_PAGE_RECORD_ADDR;

void appendFreePhysicalPage(uint32 addr, uint32 size) {
    uint32 eaddr = addr + size;
    for(uint32 phy = roundpage(addr); phy < eaddr; phy += VM_PAGE_SIZE) {
        uint32 *RecordAddr =  freePages + (freePageCount++)*4;
        *RecordAddr = phy;
        // kprintf("%dth record %8x at %8x\n", freePageCount, phy, RecordAddr);
    }
}