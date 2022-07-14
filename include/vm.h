#pragma once
#include <kernel.h>

// #define VM_DEBUG                         
#ifdef VM_DEBUG
# define DBG_PRINTF(...) kprintf(__VA_ARGS__)
#else
# define DBG_PRINTF(...)
#endif

#define VM_PAGE_SIZE                        KB(4)
#define PAGE_OFFSET_BIT                     12
#define PAGE_DIRECTORY_BIT                  10
#define PAGE_TABLE_BIT                      10
#define CR0_PG                              (1<<31)
#define CR0_WP                              (1<<16)
#define PAGE_ENTRY_SIZE                     (sizeof(void*))

#define PHY_PAGE_RECORD_ADDR                ((uint32)&end + KB(8))
#define PHY_PAGE_RECORD_END_ADDR            ((uint32)&end + KB(8)+ MB(4))

#define VM_USER_STACK_BASE                  0XFE000000
#define VM_KERNEL_STACK_BASE                PHY_PAGE_RECORD_ADDR
#define VM_HEAP_BASE                        MB(12)
#define VM_MAX_HEAP_SIZE                    MB(20)
#define VM_MAX_HEAP_ADDR                    (VM_HEAP_BASE + VM_MAX_HEAP_SIZE)
#define VM_TMP_ADDR                         PHY_PAGE_RECORD_ADDR

#define VM_PAGE_RECORD_ADDR                 MB(4)
#define VM_PAGE_TABLE0                      MB(8)        
#define VM_PAGE_DIRECTORY_ADDR              ((2 << 22) | (2 << 12))

#define getPageDirectoryEntryID(x)          ( ((uint32)x >> 22) & ((1 << PAGE_DIRECTORY_BIT)-1) )
#define getPageEntryID(x)                   ( ((uint32)x >> PAGE_OFFSET_BIT) & ((1 << PAGE_TABLE_BIT)-1) )
#define getPageOffset(x)                    ( (uint32)x & ((1 << PAGE_OFFSET_BIT)-1) )

#define PAGE_TABLE_ENTRY                    (VM_PAGE_SIZE / sizeof(void*))
#define PAGE_DIRECTORY_ENTRY                (VM_PAGE_SIZE / sizeof(void*))

#define roundpage(x)                        (((uint32)x + VM_PAGE_SIZE-1) & (~(VM_PAGE_SIZE-1)))
#define truncpage(x)                        ((uint32)x & (~(VM_PAGE_SIZE-1)))

#define RecordAddress(x)                    ( ((uint32)x / KB(4)) *4 + MB(4) )
#define Physical(x)                         ( (((uint32)x - MB(4)) / 4) * KB(4))

#define invlpg(va)                          do {                    \
                                                asm volatile(       \
                                                    "invlpg (%0);"  \
                                                    :               \
                                                    : "r"((va))     \
                                                    : "memory"      \
                                                    );              \
                                            } while(0)              \
/* Be aware of the order of the fileds */
typedef struct PageTableEntry {
    uint32 present              : 1;
    uint32 readOrWrite          : 1;
    uint32 userSupervisor       : 1;    // 1 for all users, 0 for ring 0, 1, 2 
    uint32 writeThrough         : 1;
    uint32 cacheDisabled        : 1;
    uint32 accessed             : 1;
    uint32 dirty                : 1;
    uint32 reserved             : 1;
    uint32 globalPage           : 1;
    uint32 avail                : 3;
    uint32 pageBaseAddress      : 20;
} __attribute__((packed)) PageTableEntry;

typedef struct PageDirectoryEntry {
    uint32 present                  : 1;
    uint32 readOrWrite              : 1;
    uint32 userSupervisor           : 1;    // 1 for all users, 0 for ring 0, 1, 2 
    uint32 writeThrough             : 1; 
    uint32 cacheDisabled            : 1;  
    uint32 accessed                 : 1; 
    uint32 reserved                 : 1;
    uint32 pageSize                 : 1;
    uint32 globalPage               : 1;
    uint32 avail                    : 3;
    uint32 pageTableBaseAddress     : 20;
} __attribute__((packed)) PageDirectoryEntry;

typedef PageDirectoryEntry  *PageDirectory;
typedef PageTableEntry      *PageTable;
typedef void                *Page;               
// void    recordFreePhysicalPage(uint32 addr, uint32 size);

void    enablePaging(PageDirectory pageDirectoryPhyAddr);
bool8   getPagingEnabled();
void    appendFreePhysicalPage(uint32 addr, uint32 size);
void    *allocatePhysicalPage();
void    allocateVirtualAddr(PageDirectory pageDirectory, uint32 virtualAddr, uint32 size, uint8 userSupervisor);
void    *allocateVirtualPage(PageDirectory pageDirectory, uint32 virtualAddr, uint8 userSupervisor);
void    freeVirtualAddr(PageDirectory pageDirectory, uint32 virtualAddr, uint32 size);
void    freeVirtualPage(PageDirectory pageDirectory, uint32 virtualAddr);
void    freePhysicalPage(uint32 physicalAddr);
void    initializePageTableEntry(PageTable pageTable, uint32 entryID, uint32 physicalAddr, uint8 present, uint8 userSupervisor);
void    initializePageDirectoryEntry(PageDirectory pageDirectory, uint32 entryID, uint32 physicalAddr, uint8 present, uint8 userSupervisor);
void    loadCr3(PageDirectory pageDirectoryPhyAddr);
void    setCr0();
void    clearPageTableEntry(PageTable pageTable, uint32 entryID);
PageDirectory   initialKernelPageTable();
