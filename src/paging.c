#include <paging.h>
#include <memory.h>

// Returns physical address of virtual address mapping
void * get_pagetable_entry(uint64_t virt_addr) {

    int l1 = (virt_addr >> 12) & 0x1FF;
    int l2 = (virt_addr >> (9 + 12)) & 0x1FF;
    int l3 = (virt_addr >> (9 + 9 + 12)) & 0x1FF;
    int l4 = (virt_addr >> (9 + 9 + 9 + 12)) & 0x1FF;

    uint64_t *ptr = (uint64_t *)PAGE_DIR_VIRT;
    // Checking if present in L4
    if ((ptr[l4] & PRESENT) != PRESENT) return BAD_PTR;

    ptr = (uint64_t *)(((((uint64_t)ptr << 9) | l4 << 12) & PT_LVL3) | ((uint64_t)ptr & ~PT_LVL3));
    // Checking if present in L3
    if ((ptr[l3] & PRESENT) != PRESENT) return BAD_PTR;
    if ((ptr[l3] & HUGE_PAGE) == HUGE_PAGE) {
        void *ret = (void *)(ptr[l3] & ~0xFFF); // This should be PT_LVL2 too?
        ret += (virt_addr & PT_LVL2); // Adding offset
        return ret;
        // ptr[l3] holds physical address of huge page
        // --> unmask
        // --> calc offset from virtual address, add to ret
        // --> return ret
    } 

    ptr = (uint64_t *)(((((uint64_t)ptr << 9) | l3 << 12) & PT_LVL2) | ((uint64_t)ptr & ~PT_LVL2));
    // Checking if present in L2
    if ((ptr[l2] & PRESENT) != PRESENT) return BAD_PTR;
    if ((ptr[l2] & HUGE_PAGE) == HUGE_PAGE) {
        void *ret = (void *)(ptr[l2] & ~0xFFF);
        ret += (virt_addr & PT_LVL3); // Adding offset to physical address
        return ret;
    }

    ptr = (uint64_t *)(((((uint64_t)ptr << 9) | l2 << 12) & PT_LVL1) | ((uint64_t)ptr & ~PT_LVL1));
    // Checking if present in L1
    if ((ptr[l1] & PRESENT) != PRESENT) return BAD_PTR;

    // if (mask) return (ptr[l1] & ~0xFFF);
    return (void *)(ptr[l1]); // Return entry with first 12 status bits masked out
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


    uint64_t pd = (uint64_t)PAGE_DIR_VIRT;
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

// Function to find n free spaces in page table given
// -> Recusive mapping allows function to only need to increment L1 index
// Mostly used to find new spaces in L1 anyway but can be used when new spaces in Lx are needed
void *free_page_space(uint64_t page_addr, uint16_t n) {
    // Setting increment
    int inc_free = 0, i;
    uint64_t temp_addr;
    // Increments L1 map
    for (i=0; i<512 && inc_free < n; i++, page_addr+=sizeof(page_addr)) {
        // If value is good then increment
        // Can we place in for loop with increment with i++?
        if ((*((uint64_t *)page_addr) & PRESENT) == 0) {
            // Page good for allocation

            if (inc_free == 0) temp_addr = page_addr;
            inc_free++;
        } else {
            inc_free = 0;
        }
    }
    
    if (i==511)
        return BAD_PTR;

    return (void *)temp_addr;
}


