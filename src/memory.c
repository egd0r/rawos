#include "headers/memory.h"
#include "headers/vga.h"

// Stack based
typedef struct {
    void *start;
    void *end;
    void *sp; // Stack pointer will progress to allocate frames
    int max_allocatable;
    int reserved;
} memory_regions;

memory_regions *phys_mem_regions[100]; // 100 memory regions each with stack pointers

// Bitmap based
void *BITMAP_base;
void *BITMAP_end;
//Define macro here for conversions

//

/*
    Takes MBR ADDR and returns memory map struct with memory sections

    Function maps physical pages to bitmap
*/

struct multiboot_tag_mmap * init_memory_map(void *mbr_addr) {
    // Attempting to parse multiboot information structure
    uint32_t size; // information struct is 8 bytes aligned, each field is u32 
    size = *((uint32_t *)mbr_addr); // First 8 bytes of MBR 

    struct multiboot_tag *tag;

    int largestFreeSize = 0;

    // printf("Kernel starts physaddr: %x\nEnds at physaddr: %x\n", &KERNEL_START, &KERNEL_END);
    for (tag = (struct multiboot_tag*) (mbr_addr + 8);
         tag->type != MULTIBOOT_TAG_TYPE_END;
         tag = (struct multiboot_tag*) ((multiboot_uint8_t *) tag
                                        + ((tag->size + 7) & ~7)))
    {
      switch (tag->type)
        {
            case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO:
                struct multiboot_tag_basic_meminfo *mem = (struct multiboot_tag_basic_meminfo*) tag;
                printf("Basic memory area %x - %x\n",
                       mem->mem_lower,
                       mem->mem_upper);
                break;
            case MULTIBOOT_TAG_TYPE_MMAP: 
                multiboot_memory_map_t *mmap;
                struct multiboot_tag_mmap *mmapTag = (struct multiboot_tag_mmap*) tag;

                int i=0;
                for (multiboot_memory_map_t *mmap = mmapTag->entries;
                        (multiboot_uint8_t*) mmap < (multiboot_uint8_t*) tag + tag->size; // Contiguous? Will this work? Changed from using 'tag'
                        mmap = (multiboot_memory_map_t *) ((unsigned long) mmap
                            + mmapTag->entry_size), i++)
                {
                    // printf("Memory area starting at %x with "
                            // "length of %x and type %x\n",
                            // (mmap->addr),
                            // (mmap->len),
                            // mmap->type);


                    // With each mmap, size / 4096 is set as either 1s or 0s in bitmap
                    map_physical_pages(mmap->type == 1 ? 0 : 1, mmap->len, mmap->addr);
                }

                break;
            default:
                //kprintf("Unkown tag %x, size %x\n", tag->type, tag->size);
                break;
        }

        //Mapping kernel as allocated ALWAYS - don't want corrupted bitmap
        map_physical_pages(1, &KERNEL_END-&KERNEL_START, &KERNEL_START);
    }

}

/*
    Uses global variable to iterate through bitmap and set bits as allocated
    // Length in bytes
*/

void set_bit(uint64_t addr, uint64_t bit) {
    int bitIndex = (addr % 0x40000)/PHYSICAL_PAGE_SIZE;
    int pageIndex = (addr/0x40000)-1;

    if (pageIndex > 0x20000) return; // Passed max

    ((uint64_t *)BITMAP_VIRTUAL)[pageIndex] = ((uint64_t *)BITMAP_VIRTUAL)[pageIndex] | bit<<bitIndex;
}

void map_physical_pages(int allocated, uint64_t length, uint64_t base) {
    if (!allocated) return; // Why make 0s 0s? Only run for allocations
    uint64_t *currentBitmapPtr = BITMAP_VIRTUAL; //Pointer to 64 bit chunk


    int i=0;
    //
    // DK if this could lead to still allocating 1s as free... Solution:
    //
    int numberOfPages = (length / PHYSICAL_PAGE_SIZE) + allocated;

    for (i=0; i<numberOfPages; i++) {
        set_bit(base+(i*4096), allocated);
    }

    BITMAP_end = base+i;

    
    // printf("%d actual physical pages mapped as %d\n", i, allocated);
}



void * kalloc_physical(uint64_t size) {
    // Loops through and finds free space, returns memory address

    int counter = 0;
    // Need to keep track of when counter goes from 0 -> 1 (start, value to return)
    // Need to return start of map, that is all but set bits at that physical address before with lovely function
    void *ret;
    for (int i=0; i<0x20000; i++) {
        uint64_t pages = ((uint64_t *)BITMAP_VIRTUAL)[i];
        for (int ii=0; ii<64; ii++) {
            int page = pages >> ii | pages & ~0x1; // Extract last bit
            if (page == PHYSICAL_PAGE_FREE) {
                if (counter == 0) {
                    ret = i*0x40000 + ii*4096; 
                }
                counter++;
                if (counter == size) {
                    //Map size at start
                    for (int i=0; i<size/PHYSICAL_PAGE_SIZE; i++)
                        set_bit(ret, 1);
                    return ret;
                }

            } else {
                counter=0;
            } 
        }
    }

    printf("No space for this allocation.");
    return ret;
}

// Only needs to free individual pages, virtual mem manager keeping track of physical allocations given to it!
void kfree_physical(void *ptr) {
    set_bit(ptr, 0);
}