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

// void *lowPhysicalPageSP; // For DMA?
// // Trying stack based page frame allocation approach
// void *physicalPageSP;

struct multiboot_tag_mmap *init_memory_map(void *mbr_addr);