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

/*
    Unmap identity mapped regions
*/
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

// Creates heap of size ? 
void * create_heap() {

}

void * kalloc(size_t size) {

}

// When out of pages, allocate with kalloc_physical macro and add to page entry
uint64_t allocate_page() { // Walk pages and return address of free page? Will allocate contiguous pages
                        // If allocation is unsuccessful, then NULL is returned

    uint64_t pageSize = 0x40000000; // Start with 1GiB HUGEPAGE - indexed at L3
                                    // L2 indexes - 2MiB
                                    // L1 indexes - 4KiB

    uint64_t virtualAddress = 0;       
    // Loops through page directory
    (uint64_t **)page_dir = (*((uint64_t *)PAGE_DIR_VIRT));

    (uint64_t **)page_table_entryl3;

    for (int i=0; i<512*8 || !(page_dir & PRESENT); i++) {
        page_table_entryl3 = page_dir[]
    }

    

    // 512 entries in each page table - just check present flag
}

// Recursively allocates pages
// Takes index of page table to search
// Returns free space
uint64_t allocate_page_rec(uint64_t **pt, uint64_t addr) {

    
    for (int i=0; i<512; i++) {
        if (pt[i] & HUGE_PAGE) { // If this page is hugepage
            addr << i;
            return addr;
        }
    }
}

// Last entry of PML4 is mapped to itself allowing easy index into all page tables


// Function creates virtual address from entries into page tables
void *vaddr_from_entries(size_t pml4e, size_t pdpte, size_t pde, size_t pte, size_t offset) {
    return (void *)((pml4e << 39) | (pdpte << 30) | (pde << 21) | (pte << 12) | offset);
}