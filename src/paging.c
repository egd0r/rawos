#include "headers/paging.h"
#include "headers/memory.h"


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
uint64_t allocate_pages(size_t n) { // Walk pages and return address of free page? Will allocate contiguous pages

    uint64_t *page_dir_ptr = PAGE_DIR_VIRT; //Used for indexing P4 directly
    int i=0;
    // Loop through L4 looking for present L3
    for (i=0; i<511 && ((page_dir_ptr[i] & PRESENT == 0)); page_dir_ptr++);

    //i holds index into L4
    //i must be placed: 0o177777_777_777_777_000_0000 fucking octal
    //                  0xFFFF FFFF FFE0 0000 ; E = 1110
    uint64_t *page_l3 = (PAGE_DIR_VIRT & ~0x7FFFF) & i<<12; // Unsetting index and oring l3_index 

    for (i=0; i<511 && ((page_l3[i] & PRESENT == 0)); page_l3++);
    
    uint64_t *page_l2 = (PAGE_DIR_VIRT & ~0xFFFFFFF) & i<<(12+9); // Unsetting index and oring l3_index 

    for (i=0; i<511 && ((page_l2[i] & PRESENT == 0)); page_l2++);

    uint64_t *page_l1 = (PAGE_DIR_VIRT & ~0x7FFFFFFFFF) & i<<(12+9+9); // Unsetting index and oring l3_index 

    // 512 entries in each page table - just check present flag
}

// Recursively allocates pages
// Takes index of page table to search
// Returns free space
uint64_t allocate_page_rec(uint64_t *pt, uint64_t addr) {

    
    for (int i=0; i<512; i++) {
        if (pt[i] & HUGE_PAGE) { // If this page is hugepage
            addr << i;
            return addr;
        }
    }
}


// Function creates virtual address from entries into page tables
void *vaddr_from_entries(size_t pml4e, size_t pdpte, size_t pde, size_t pte, size_t offset) {
    return (void *)((pml4e << 39) | (pdpte << 30) | (pde << 21) | (pte << 12) | offset);
}

// Function to find n free spaces in page table given
// -> Recusive mapping allows function to only need to increment L1 index
// Mostly used to find new spaces in L1 anyway but can be used when new spaces in Lx are needed
void *free_page_space(uint64_t page_addr, uint16_t n) {
    // Setting increment
    int inc_free = 0, i;

    // Increments L1 map
    for (i=0; i<512 || inc_free < n; i++, page_addr++) {
        // If value is good then increment
        // Can we place in for loop with increment with i++?
        if ((*((uint64_t *)page_addr) & PRESENT) == 0) {
            // Page good for allocation
            inc_free++;
        } else {
            inc_free = 0;
        }
    }
    
    if (i==511)
        return BAD_PTR;

    return page_addr;
}


