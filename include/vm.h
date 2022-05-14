#pragma once
#include <kernel.h>

#define VM_PAGE_SIZE                        4096
#define PHYSICAL_PAGE_RECORD_ADDR           ((uint32)&end + KB(8))
#define PHYSICAL_PAGE_RECORD_END_ADDR       ((uint32)&end + KB(8)+ MB(4))
#define roundpage(x)                        (((uint32)x + VM_PAGE_SIZE-1) & (~(VM_PAGE_SIZE-1)))
#define truncpage(x)                        ((uint32)x & (~(VM_PAGE_SIZE-1)))
#define RecordAddress(x)                    ( (x/KB(4)) *4 + MB(4) )
#define Physical(x)                         ( ((x - MB(4)) / 4) * KB(4))

void recordFreePhysicalPage(uint32 addr, uint32 size);
void appendFreePhysicalPage(uint32 addr, uint32 size);