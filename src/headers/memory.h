#pragma once
#include "paging.h"
#include "multiboot2.h"
#include "types.h"

#define KERNEL_MAX_PHYS 0x1200000
#define KERNEL_OFFSET  0xFFFFFFFF80000000 // 511 + 510 + L2 + L1 + Offset
#define BITMAP_VIRTUAL 0xFFFFFFFF81000000 // 2MB bitmap - 1 in index 8
#define BITMAP_MAX     0xFFFFFFFF81200000 // Next index in page table - 8 + 1
#define PHYSICAL_PAGE_SIZE 4096
#define PHYSICAL_PAGE_ALLOCATED 1
#define PHYSICAL_PAGE_FREE      0

// For extracting indexes of page tables in virtual addresses
//     Mapping indexes
// Generalise function 
#define PT_LVL4 0x0
#define PT_LVL3 0x1FFFFF 
#define PT_LVL2 0x3FFFFFFF
#define PT_LVL1 0x7FFFFFFFFF

// Physical address of start and end of kernel
extern uint64_t KERNEL_START;
extern uint64_t KERNEL_END;

#define KERNEL_LVL2_MAP 0xFFFFFF7FBFFFE000

struct multiboot_tag_mmap *init_memory_map(void *mbr_addr);

#define KALLOC_PHYS() kalloc_physical(1)
#define sbrk(n) page_alloc(KERNEL_LVL2_MAP, PT_LVL2, n % PHYSICAL_PAGE_SIZE == 0 ? n/PHYSICAL_PAGE_SIZE : n/PHYSICAL_PAGE_SIZE + 1) | KERNEL_OFFSET

void * kalloc_physical(size_t size); //Sets n physical pages as allocated and returns physical address to be placed in page table
void kfree_physical(void *ptr);
void memset(uint64_t ptr, uint8_t val, uint64_t size);
// LVLs defined above
#define p_alloc(pt_ptr, n) page_alloc(pt_ptr, PT_LVL4, n)
// void *page_alloc(uint64_t *pt_ptr, int LVL, uint16_t n);

void *new_malloc(int bytes);
void new_free(void *ptr);
void print_list();


// TODO -- requred my mymalloc
#define PROT_WRITE    0x01
#define PROT_READ     0x02
#define MAP_SHARED    0x04
#define MAP_ANONYMOUS 0x08

// Allows heap to be non-contiguous in physical memory
// Will try to allocate contigious pages
// typedef struct HEAP {
//     void *startOfHeap;
//     struct HEAP *nextHeap;
// } heap_t;

// heap_t heapStart; //Starts at end