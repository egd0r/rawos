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

typedef struct PageDirectoryEntry {
    // uint8_t present;
    // uint8_t rw;
    // uint8_t userSuper;
    // uint8_t writeThrough;
    // uint8_t cacheDisabled;
    // uint8_t accessed;
    // uint8_t ignore;
    // uint8_t hugePages;
    // uint8_t flags;
    // uint8_t ignore1;
    // uint8_t available;
    uint64_t addr;
} pde_t __attribute__((packed));

typedef struct PageTable {
    pde_t entries[512];
} pt_t __attribute__((aligned(0x1000)));


void get_physaddr(void *virt_addr);
void unmap_page(unsigned long long virt_addr);

void *free_page_space(uint64_t page_addr, uint16_t n);



/*
    malloc:
        -> sbrk() increases heap size
        -> allocate new page
        -> new free space

*/