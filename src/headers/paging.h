#pragma once
#include "types.h"
#define PAGE_DIR_VIRT 0xFFFFFF7FBFDFE000 // 2nd last entry maps to itself
                    //0xFFFFFFFFC0000000
/*                                L3        L2        L1          
    Page Dir Virt = 0xFFFF  1111 1111 1111 1111 1111 1111 11    
                             F    F   F     F   F     F    C
    Page Dir itself is mapped to a 4kB page
*/                  

#define PRESENT     0x01
#define RW          0x02
#define HUGE_PAGE   0x80
#define FLAGS       0xFFF
#define BAD_PTR     (uint64_t)0xDEADBEEF

typedef struct PageTable {
    uint64_t entries[512];
} pte;

void * get_pagetable_entry(uint64_t virt_addr);
void unmap_page(unsigned long long virt_addr);

void * free_page_space(uint64_t page_addr, uint16_t n);



/*
    malloc:
        -> sbrk() increases heap size
        -> allocate new page
        -> new free space

*/