#include "headers/paging.h"


void get_physaddr(void *virt_addr) {                         
    // unsigned long pdindex = (unsigned long)virt_addr >> 39   & 0x0000FF8000000000;
    // unsigned long ptl3index = (unsigned long)virt_addr >> 30 & 0x0000007FC0000000;
    // unsigned long ptl2index = (unsigned long)virt_addr >> 21 & 0x000000002FE00000;
    // unsigned long ptl1index = (unsigned long)virt_addr >> 12 & 0x00000000001FF000;

    unsigned long pdindex = ((unsigned long)virt_addr & 0x0000FF8000000000) >> 39;
    unsigned long ptl3index = ((unsigned long)virt_addr & 0x0000007FC0000000) >> 30;
    unsigned long ptl2index = ((unsigned long)virt_addr & 0x000000002FE00000) >> 21;
    unsigned long ptl1index = ((unsigned long)virt_addr & 0x00000000001FF000) >> 12;

    // Extract metadata from each index into next page    

    // Have to check hugepage flags

}

void unmap_page(unsigned long long virt_addr) {

    // Deconstruct virtual address
    // Index page directory to check mapping

    uint16_t pdIndex = ((unsigned long)virt_addr & 0x0000FF8000000000) >> 39; // Max 9 bits


    // unsigned long long *pd = (unsigned long long *)PAGE_DIR_VIRT;
    // unsigned long long ptr = pd[pdIndex]; // Check flags on this ptr FAULT
    
    
    // unsigned long long l3index = ((unsigned long)virt_addr & 0x0000007FC0000000) >> 30;


    uint64_t pd = (uint64_t *)PAGE_DIR_VIRT;
    uint16_t flags;
    do {
        pd = ((uint64_t *)(pd & ~FLAGS))[pdIndex]; // Deref masked out flags
    } while ((pd & PRESENT) && !(pd & HUGE_PAGE));

    if (!(pd & PRESENT)) {
        // printf("This entry is not present!");
        return;
    }

    pd &= (~PRESENT); //Removing present flag, invalidating entry

    return;

}

void *allocate_page() { // Walk pages and return address of free page?

}