#pragma once
#include "types.h"
#define PAGE_DIR_VIRT 0xFFFFFFFFBFFFF000 // Last entry in L1 maps to L4, indexing this address gives PDEs
                    //0xFFFFFFFFC0000000
/*                                L3        L2        L1          
    Page Dir Virt = 0xFFFF  1111 1111 1111 1111 1111 1111 11    
                             F    F   F     F   F     F    C
    Page Dir itself is mapped to a 4kB page
*/                  

#define PRESENT     0x01
#define RW          0x02
#define HUGE_PAGE   0x08
#define FLAGS       0xFFF

void get_physaddr(void *virt_addr);
void unmap_page(unsigned long long virt_addr);

// Uses malloc to allocate heap
// Must be contiguous in virtual memory... space for malloc
void * create_heap(); 

void * kalloc(size_t size);
void free(void *ptr); // References allocation with size

// Allows heap to be non-contiguous in physical memory
// Will try to allocate contigious pages
typedef struct HEAP {
    void *startOfHeap;
    heap_t *nextHeap;
} heap_t;

heap_t heapStart;

/*
    malloc:
        -> sbrk() increases heap size
        -> allocate new page
        -> new free space


*/