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

    printf("Kernel starts phyaddr: %x\nEnds at physaddr: %x\n", &KERNEL_START, &KERNEL_END);
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
                    printf("Memory area starting at %x with "
                            "length of %x and type %x\n",
                            (mmap->addr),
                            (mmap->len),
                            mmap->type);


                    // With each mmap, size / 4096 is set as either 1s or 0s in bitmap

    
                    if ( (mmap->type == 1) && (mmap->len > largestFreeSize) ) {
                        largestFreeSize = mmap->len;
                        BITMAP_base = mmap->addr;
                        BITMAP_end = mmap->addr + mmap->len;
                    }


                    


                    // TRYING STACK BASED APPROACH HERE?? UNFINISHED - REFER TO BOOK OF Qs
                    phys_mem_regions[i]->start = mmap->addr;
                    phys_mem_regions[i]->sp = mmap->addr;
                    phys_mem_regions[i]->end = mmap->addr + mmap->len;

                    if (mmap->type != 1)
                        phys_mem_regions[i] = 1;


                }

                break;
            default:
                //kprintf("Unkown tag %x, size %x\n", tag->type, tag->size);
                break;
        }
    }

    map_physical_pages(1, largestFreeSize);
}

/*
    Uses global variable to iterate through bitmap and set bits as allocated
*/
uint64_t *currentBitmapPtr = BITMAP_VIRTUAL; //Pointer to 64 bit chunk
void map_physical_pages(int allocated, int length) {

    int i=0;
    for (i=0; i<length/PHYSICAL_PAGE_SIZE/64; i++, currentBitmapPtr++) {
        *currentBitmapPtr = allocated == 1 ? PHYSICAL_PAGE_ALLOCATED : PHYSICAL_PAGE_FREE;
    }
    
    printf("Iterating through %d chunks of 8 bytes\n", length/PHYSICAL_PAGE_SIZE/64);
    printf("%d physical pages mapped as %d\n", i*64, allocated);
}

void kalloc_physical(uint64_t size); //Sets n physical pages as allocated and returns physical address to be placed in page table
void kfree_physical(uint64_t size);