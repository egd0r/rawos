#pragma once
#include "multiboot2.h"
#include "types.h"

#define BITMAP_VIRTUAL 0xFFFFFFFF80240000 // 2MB bitmap
#define BITMAP_MAX     0xFFFFFFFF80250000 // Next index in page table
#define PHYSICAL_PAGE_SIZE 4096
#define PHYSICAL_PAGE_ALLOCATED 0xFFFFFFFFFFFFFFFF
#define PHYSICAL_PAGE_FREE      0

// Physical address of start and end of kernel
extern uint64_t KERNEL_START;
extern uint64_t KERNEL_END;

struct multiboot_tag_mmap *init_memory_map(void *mbr_addr);

#define KALLOC_PHYS() kalloc_physical(1)

void * kalloc_physical(size_t size); //Sets n physical pages as allocated and returns physical address to be placed in page table
void kfree_physical(void *ptr);