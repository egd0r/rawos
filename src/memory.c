#include <memory.h>
#include <vga.h>
#include <fs.h>

// Bitmap based
void *BITMAP_base;
void *BITMAP_end;
//Define macro here for conversions
uint64_t initrd_start;

void memcpy(uint8_t *from, uint8_t *to, uint64_t size) {
    for (int i=0; i<size; i++) {
        to[i] = from[i];
    }
}

//
void *mmap(int argv, ...) {
    return NULL;
}

void munmap(void *ptr, int size) {
    return;
}

void *sbrk(int n) {
    int ret = heap_current;
    heap_current += n;
    return ret;
}

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

    // kprintf("Kernel starts physaddr: %x\nEnds at physaddr: %x\n", &KERNEL_START, &KERNEL_END);
    for (tag = (struct multiboot_tag*) (mbr_addr + 8);
         tag->type != MULTIBOOT_TAG_TYPE_END;
         tag = (struct multiboot_tag*) ((multiboot_uint8_t *) tag
                                        + ((tag->size + 7) & ~7)))
    {
      switch (tag->type)
        {
            case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO:
                struct multiboot_tag_basic_meminfo *mem = (struct multiboot_tag_basic_meminfo*) tag;
                kprintf("Basic memory area %x - %x\n",
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
                    // kprintf("Memory area starting at %x with "
                    //         "length of %x and type %x\n",
                    //         (mmap->addr),
                    //         (mmap->len),
                    //         mmap->type);


                    // With each mmap, size / 4096 is set as either 1s or 0s in bitmap
                    map_physical_pages(
                        (mmap->type == 1) ? 0 : 1
                        , mmap->len, mmap->addr);
                }

                break;
            case MULTIBOOT_TAG_TYPE_MODULE:
                struct multiboot_tag_module *initrd = (struct multiboot_tag_mmap*) tag;
                // initrd += KERNEL_OFFSET; // Mapping to higher space
                initrd_start = initrd->mod_start;

                kprintf("Module start: %d\n", initrd->mod_start);
                kprintf("Module end: %d\n", initrd->mod_end);
                kprintf("Module size: %d\n", initrd->size);
                kprintf("Module type: %d\n", initrd->type);
                int no = get_number_of_files(initrd->mod_start);
                kprintf("Number of files in the module %d\n", no);

            default:
                kprintf("Unkown tag %x, size %x\n", tag->type, tag->size);
                break;
        }

    }
    //Mapping kernel as allocated ALWAYS - don't want corrupted bitmap
    // map_physical_pages(1, 0x200000*9, 0); // Allocating what's already been mapped in HUGEPAGES - so overkill lol
    kprintf("%x %x\n", &KERNEL_START, &KERNEL_END);

}

/*
    Uses global variable to iterate through bitmap and set bits as allocated
    // Length in bytes
*/

// For allocating one page of physical memory, most likely used
void set_bit(uint64_t addr, uint64_t bit) {
    int bitIndex = (addr % 0x40000)/PHYSICAL_PAGE_SIZE;
    int pageIndex = (addr/0x40000);//-1;

    if (pageIndex > 0x20000) return; // Passed max


    if (bit == 1)
        ((uint64_t *)BITMAP_VIRTUAL)[pageIndex] = (uint64_t)((uint64_t *)BITMAP_VIRTUAL)[pageIndex] | (uint64_t)1<<bitIndex;
    else
        ((uint64_t *)BITMAP_VIRTUAL)[pageIndex] = (uint64_t)((uint64_t *)BITMAP_VIRTUAL)[pageIndex] ^ (uint64_t)1<<bitIndex;
}

void map_physical_pages(int allocated, uint64_t length, uint64_t base) {
    if (allocated) return; // Why make 1s 1s? Only run for de-allocations
    
    while (base < KERNEL_MAX_PHYS) {
        base += 0x1000; //Add 4096 to base until outside kernel space allocated already
    }
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

    
    // kprintf("%x %d actual physical pages mapped as %d\n", base, i, allocated);
}

// For contiguous physical allocations, DMA etc
// Size is multiples of 4096
void * kalloc_physical(size_t size) {
    // Loops through and finds free space, returns memory address

    int counter = 0;
    // Need to keep track of when counter goes from 0 -> 1 (start, value to return)
    // Need to return start of map, that is all but set bits at that physical address before with lovely function
    void *ret;
    for (int i=0; i<0x20000; i++) {
        uint64_t pages = ((uint64_t *)BITMAP_VIRTUAL)[i];
        for (int ii=0; ii<64; ii++) {
            uint8_t page = (pages >> ii) & 0x1; // Extract last bit
            if (page == PHYSICAL_PAGE_FREE) {
                if (counter == 0) {
                    ret = i*0x40000 + ii*4096; 
                }
                counter++;
                if (counter == size) {
                    //Map size at start
                    for (int i=0; i<((size%PHYSICAL_PAGE_SIZE == 0) ? size/4096 : 1+(size/4096)); i++)
                        set_bit(ret, 1);
                    return ret;
                }

            } else {
                counter=0;
            } 
        }
    }

    kprintf("No space for this allocation.");
    return ret;
}

// Only needs to free individual pages, virtual mem manager keeping track of physical allocations given to it!
void kfree_physical(void *ptr) {
    set_bit(ptr, 0);
}

void memset(uint64_t ptr, uint8_t val, uint64_t size) {
    for (int i=0; i<size; i++) {
       *((uint8_t *)((uint64_t)ptr+i)) = val;
    }
}

// Function to find free space of size n in page tables and return space to them

// Essentially can be used as sbrk with malloc, just need managing heap space, linked list
void *allocate_page(size_t size) {
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

    uint64_t page_l1 = (PAGE_DIR_VIRT & ~0x7FFFFFFFFF) & i<<(12+9+9); // Unsetting index and oring l3_index 

    uint64_t *space = (uint64_t *)free_page_space(page_l1, size);

    if (space == BAD_PTR) {
        kprintf("uh oh!!!\n");
        return;
    }

    // We can safely allocate these pages space
    for (int i=0; i<size; i++) {
        uint64_t phys_addr = (uint64_t)KALLOC_PHYS();
        space[i] = phys_addr |= (PRESENT | RW); // Giving full perms for now
    }

    return space;
}

// Takes page table index and level of current table, searches lvl levels
// Takes NUMBER OF PAGES TO ALLOCATE
uint64_t previous_allocation = NULL;
uint64_t page_alloc(uint64_t *pt_ptr, uint64_t LVL, uint16_t n) {
    if (n == 0) {
        return previous_allocation;
    } 

    if (LVL == PT_LVL1) {
        // If we're at end walk final table and return free space
        uint64_t *space = (uint64_t *)free_page_space(pt_ptr, n);

        if (space == BAD_PTR) {
            kprintf("uh oh!!!\n");
            return NULL;
        }

        // We can safely allocate these n pages space
        for (int i=0; i<n; i++) {
            space[i] = ((uint64_t)KALLOC_PHYS()) | (PRESENT | RW); // Giving full perms for now
        }

        // Can't return pointer to page space but to memory that corresponds to it
        return 0 | (space - pt_ptr) << 12;
    }
    // First level will be 4
    uint64_t *new_pt_ptr;
    /*
        Switch-case process:
            - checks current level, loops through current level and
            - calls next level on current using index 
            - for each level, recursively call allocate function
            - when a free space of size n is found in the page table:
                - Physical addresses are allocated to it
                - Index is returned, loop is broken if NULL is not returned
    */
    int shift, new=0;
    void *free_space;
    for (int i=0; i<511; i++) {
        // Just continue if anything comes up as hugepage
        // Can add allocations for HUGE_PAGE later
        if ((pt_ptr[i] & HUGE_PAGE) == HUGE_PAGE) {
            continue;
        } 
        // If place is null, allocate a new page to it since we're not at lowest level yet
        if ((pt_ptr[i] == 0)) {
            pt_ptr[i] = ((uint64_t)KALLOC_PHYS()) | PRESENT | RW; // Allocate physical page to space
            new = 1;
        }
        switch (LVL) {
            case PT_LVL2:
                // Find ptr of level 1 with index
                new_pt_ptr = ((((uint64_t)pt_ptr << 9) | i << 12) & PT_LVL1) | ((uint64_t)pt_ptr & ~PT_LVL1);
                // new_pt_ptr = (PAGE_DIR_VIRT & ~PT_LVL1) | i<<(12+9+9); // Unsetting index and oring l3_index 
                // If below returns NOT NULL then index i can be used to
                // add to virtual address
                if (new == 1) memset(new_pt_ptr, 0, 512);
                free_space = page_alloc(new_pt_ptr, PT_LVL1, n);
                shift = 12+9;
            break;
            case PT_LVL3:
                // Find ptr of level 2 with index
                new_pt_ptr = ((((uint64_t)pt_ptr << 9) | i << 12) & PT_LVL2) | ((uint64_t)pt_ptr & ~PT_LVL2);
                // new_pt_ptr = (PAGE_DIR_VIRT & ~PT_LVL2) | i<<(12+9); // Unsetting index and oring l3_index 
                if (new == 1) memset(new_pt_ptr, 0, 512);
                free_space = page_alloc(new_pt_ptr, PT_LVL2, n);
                shift = 12+9+9;
            break;
            case PT_LVL4:
                if (i==510) continue; // Don't want to go here
                // Find ptr of level 3 with index
                new_pt_ptr = ((((uint64_t)pt_ptr << 9) | i << 12) & PT_LVL3) | ((uint64_t)pt_ptr & ~PT_LVL3);
                if (new == 1) memset(new_pt_ptr, 0, 512);
                // new_pt_ptr = (PAGE_DIR_VIRT & ~PT_LVL3) | i<<12; // Unsetting index and oring l3_index 
                free_space = page_alloc(new_pt_ptr, PT_LVL3, n);
                shift = 12+9+9+9;
            break;
            default:
                continue;
            break;
        }
        // If there's no space in the page table at this index
        // if (free_space == NULL) {
        //     continue;
        // }

        // Return virtual address of this page table + last page table
        // Casting pointer to number so it can be operated on
        uint64_t ret = ((uint64_t)free_space | (i << shift));
        previous_allocation = ret;
        return ret;
    }

    // Return null if at end of iteration, no space in current table - if all are hugepages lol
    return NULL;
}

// Increases size of heap, returns ptr to free 8k essentially
// -- to control where allocations lie essentially

// This can be turned to regular sbrk if there's a way to get the start of heap pointer?
// Plan to abstract over virtual allocations with heap so they don't need to be contiguous pages
//// For now, simple - allocating space for this??
void k_sbrk() {

}

// Malloc implementation

//Setting default bytes to request from the heap
#define DEFAULT_BYTES 8192
//Bit mask on size to represent allocated
#define ALLOCATED_MASK 0x40000000 
//Number of bins defined in the bin list below
#define NUMBER_OF_BINS 8

//Struct for information about chunks in memory
typedef struct CHUNK {
    int prev_size;   
    int size;        

    struct CHUNK *next; 
    struct CHUNK *prev; 
} chunk_n; 

//Bins that hold free lists holding free spaces of binary mutiples
typedef struct BIN {
    int bin_min; 
    int bin_max;
    int bin_size; //Total free space in this bin

    chunk_n *chunk_list_head; //Head of chunk list held in this bin
    struct BIN *next_bin; //Points to bin of bigger size
} bin_n;

const int max_sbrk_size = DEFAULT_BYTES-(sizeof(chunk_n)*3);
chunk_n *tail_of_last_heapchunk = NULL;

//Bins not stored in heap as to not take up unnecessary space when they're static
bin_n *bin_head;
bin_n bin_list[NUMBER_OF_BINS] = { 
    { .bin_min=-1, .bin_max=63, .bin_size=0, .chunk_list_head=NULL },
    { .bin_min=64, .bin_max=127, .bin_size=0, .chunk_list_head=NULL },
    { .bin_min=128, .bin_max=255, .bin_size=0, .chunk_list_head=NULL },
    { .bin_min=256, .bin_max=511, .bin_size=0, .chunk_list_head=NULL },
    { .bin_min=512, .bin_max=1023, .bin_size=0, .chunk_list_head=NULL },
    { .bin_min=1024, .bin_max=2047, .bin_size=0, .chunk_list_head=NULL },
    { .bin_min=2048, .bin_max=4095, .bin_size=0, .chunk_list_head=NULL },
    { .bin_min=4096, .bin_max=INT_MAX, .bin_size=0, .chunk_list_head=NULL },
}; //        ^ no upper limit on final bin, for heap chunk coalescing ^

//For keeping track of mmaped space so program knows to call munmap to free and not look in heap
chunk_n *mmap_space_head; 

//Returns 1 if given chunk is allocated, 0 otherwise
int is_allocated(chunk_n *chunk) {
    if (!chunk) return -1;
    return ((chunk->size & ALLOCATED_MASK)==ALLOCATED_MASK);
}   

//Returns next chunk of given chunk
void *next_chunk(chunk_n *passedChunk) {
    if (!passedChunk) return NULL;
    return ((void *)passedChunk)+(passedChunk->size & ~(ALLOCATED_MASK))+sizeof(chunk_n);
}

//Returns previous chunk of given chunk
void *prev_chunk(chunk_n *passedChunk) {
    if (!passedChunk) return NULL;
    return ((void *)passedChunk)-((passedChunk->prev_size & ~(ALLOCATED_MASK))+sizeof(chunk_n));
}

//Takes chunk and updates the next chunk in memory's prev_size with new size for chunk
//Important to ensure chunks contain correct information for coalescing
void update_prev_size(chunk_n *toCheck) {
    if (!toCheck) return;
    //Find next chunk
    chunk_n *nextChunk = next_chunk(toCheck);
    nextChunk->prev_size = toCheck->size;
}

//Remove given item from given linked list
//Returns item removed
chunk_n *remove_from_list(chunk_n **head_of_list, chunk_n *toRemove) {
    if (!*head_of_list || !toRemove) return NULL;

    if (toRemove->prev) toRemove->prev->next = toRemove->next;
    else *head_of_list = toRemove->next;
    if (toRemove->next) toRemove->next->prev = toRemove->prev;
    toRemove->next = NULL; toRemove->prev = NULL;
    return toRemove;
}

//Inserting chunk at head of list given
//Returns 1 when successful, 0 otherwise
int place_in_list(chunk_n **head_of_list, chunk_n *toAdd) {
    if (!toAdd) return 0;

    if (*head_of_list) {
        (*head_of_list)->prev = toAdd;
        toAdd->next = *head_of_list;
    }

    *head_of_list = toAdd;
    return 1;
}


//Searches bins for placing a certain size of bytes
//Returns NULL if size fits no bins or returns bin where size should be placed otherwise
//Good for placing free chunk in bins, bad for getting possible free chunk
//Can return empty bin
bin_n *search_bins(int size) {
    for (int i=0; i<NUMBER_OF_BINS; i++) {
        //Checks if size is within the particular bin range
        if ((bin_list[i].bin_min < size) && (size <= bin_list[i].bin_max)) {
            return &bin_list[i];
        }
    }
    return NULL;
}

//Will search bins above size of bytes
//Good for getting possible free chunk
//Cannot return empty bin
chunk_n *search_free(int bytes) {
    for (int i=0; i<NUMBER_OF_BINS; i++) {
        //Check determines if free space could exist within bin that bytes could take up
        if ((bytes <= bin_list[i].bin_max) && bin_list[i].chunk_list_head) {
            chunk_n *temp = bin_list[i].chunk_list_head;
            //This for loop searches for the first free chunk of enough size within the bin to return
            for (; temp; temp=temp->next) {
                //first space that will fit will be returned 
                if ((!is_allocated(temp)) && (temp->size >= bytes)) {
                    return temp;
                }
            }
        }
    }
    //If no space is found, NULL is returned
    return NULL;
}

//Removes specified chunk from bin it's in
chunk_n *remove_from_bin(chunk_n *toRemove) {
    //Find bin that chunk is in with size of chunk
    bin_n *binRemove = search_bins(toRemove->size);
    //Call remove procedure which returns item removed or NULL
    chunk_n *removed = remove_from_list(&binRemove->chunk_list_head, toRemove);

    if (removed) {
        //Increment bin size if removal succeeds
        binRemove->bin_size-=removed->size;
    }

    return removed;
}

//Place free chunk in correct bin
int place_in_bin(chunk_n *toPlace) {

    //Finding correct bin toPlace needs to be placed
    bin_n *binPlace = search_bins(toPlace->size);
    if (!binPlace) {
        return 0;
    }
    //Placing chunk in list
    if (place_in_list(&binPlace->chunk_list_head, toPlace)) {
        //If successful, incrementing total size of bin
        binPlace->bin_size+=toPlace->size;
        return 1;
    }
    
    return 0;
}

//Function to merge end chunk to start chunk
//Returns combined chunk if successful, NULL otherwise
chunk_n * combine(chunk_n *start, chunk_n *end) {
    //Condition determines if chunks can be combined
    if ( (is_allocated(start) || (is_allocated(end))) ) return NULL;
    //These chunks will ALWAYS be next to each other in memory
    //These chunks are also in the correct bins so must both be removed to be combined
    remove_from_bin(start);
    remove_from_bin(end);
    //Increasing size of remaining chunk to include removed chunk
    start->size += end->size + sizeof(chunk_n);

    kprintf("Coalesced %x with %x, inserting into correct bin!\n", start, end);
    //Replacing remaining chunk into bin
    place_in_bin(start);
    return start;
}

//Returns pointer to mmap space
//For servicing large memory allocations
void *create_new_mmap_space(int bytes) {
    //Creating metadata for new memory chunk
    chunk_n newAllocated = { .prev_size=0, .size=(bytes | (ALLOCATED_MASK)), .next=NULL, .prev=NULL };

    //Asking OS for memory of a specific size
    void *newMemPtr = mmap(NULL, bytes+sizeof(chunk_n), PROT_WRITE | PROT_READ, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    
    *((chunk_n *)newMemPtr) = newAllocated;
    kprintf("\tMemory provided by mmap at: %x\n", newMemPtr);


    return newMemPtr;
}

//Function for creating a new heap space
//Returns pointer to start of free chunk created within the heap space
void *create_new_heap_space() { 
    //Condition checks whether new heap is adjecent in memory to previous heap created
    //Allows coalescing of heap spaces
    void *prev = sbrk(0);
    //Creating heap metadata
    // chunk_n tail_heap = { .size=(0 | (ALLOCATED_MASK)), .next=NULL };
    chunk_n tail_heap;
    tail_heap.next = NULL;
    tail_heap.size = 0 | ALLOCATED_MASK;

    // chunk_n head_heap = { .size=(0 | (ALLOCATED_MASK)), .prev=tail_of_last_heapchunk };
    chunk_n head_heap;
    head_heap.size = 0 | ALLOCATED_MASK;
    head_heap.prev = tail_of_last_heapchunk;
    head_heap.prev_size = tail_of_last_heapchunk == NULL ? NULL : tail_of_last_heapchunk->size;
    //Creating new chunk metadata       note: size defined with verbosity for clarity
    chunk_n newFreeChunk = { .prev_size = head_heap.size, .size = DEFAULT_BYTES-(sizeof(chunk_n)+sizeof(head_heap)+sizeof(tail_heap)), .next = NULL, .prev = NULL };

    //Calling sbrk to advance the heap pointer
    void *newMemPtr = sbrk(DEFAULT_BYTES);
    void *tailInsertion = newMemPtr+(DEFAULT_BYTES-sizeof(tail_heap));
    //Linking head to tail and tail to head
    head_heap.next=(chunk_n *)(tailInsertion);
    tail_heap.prev=(chunk_n *)(newMemPtr);
    //Placing heap metadata
    *((chunk_n *)newMemPtr) = head_heap;
    *((chunk_n *)tailInsertion) = tail_heap;
    //Resetting pointer to final heap tail
    tail_of_last_heapchunk = tailInsertion;


    //Calculating space of new free chunk
    void *firstFreeLoc = newMemPtr+=sizeof(head_heap);
    //Placing new free chunk in memory
    *((chunk_n *)firstFreeLoc) = newFreeChunk;

    //Returning free chunk ready to be split by new_malloc
    return newMemPtr;
}

//Allocates a space of size "bytes" and returns a pointer to the start of the chunk
//Has the ability to call mmap for allocations larger than those that are able to fit in the free chunks available.
void *new_malloc(int bytes) {

    //Initialising pointer to free space
    chunk_n *foundFree;
    //Free chunks can be greater than 8k in size from adjecent heap spaces
    if (!(foundFree = search_free(bytes))) {
        if (bytes > max_sbrk_size) {
            foundFree = (chunk_n *)create_new_mmap_space(bytes);
            place_in_list(&mmap_space_head, foundFree);
            return ((void *)foundFree+sizeof(chunk_n));
        } else {
            //Conditions satisfied for advancing heap pointer with sbrk
            foundFree = (chunk_n *)create_new_heap_space();
        }
    }

    if (!foundFree) {
        kprintf("ERROR with allocation, please consult Manuel\n");
        return NULL;
    }

    //Remove foundFree from the bin it's in, operation only succeeds if chunk present in bin
    //returns NULL otherwise (in the case of having allocated new space (heap or mmap)
    remove_from_bin(foundFree);

    //Create allocated chunk
    chunk_n newChunk = { .prev_size = 0, .size = bytes | ALLOCATED_MASK, .next = NULL, .prev = NULL };

    newChunk.prev_size=foundFree->prev_size;
    //Defining return value
    void *ret = ((void *)foundFree)+sizeof(chunk_n);

    //Setting new size of foundFree
    foundFree->size-=(bytes+sizeof(chunk_n));

    //Does the remaining free space fit into any bin?
    if (!search_bins(foundFree->size)) {
        //We don't care about foundFree bin if it doesn't fit so we ignore it
        //Have to make sure allocator knows it has the right information to find next and prev
        //chunks when coalescing
        //ensuring size allocated chunk is updated, assists with coalescing
        newChunk.size += sizeof(chunk_n)+foundFree->size;
        *foundFree = newChunk;
        update_prev_size(foundFree);
        return ret;
    } 
    //If free chunk remains then we modify its position and replace it

    chunk_n *newSpaceForOldPtr = ((chunk_n *)((((void *)(foundFree))+(bytes+sizeof(chunk_n)))));
    
    //Placing remaining free chunk further up the heap space
    *newSpaceForOldPtr = *foundFree;
    //To ensure the heap after contains the correct prev_size
    update_prev_size(newSpaceForOldPtr);

    //Replacing remaining free into bin
    place_in_bin(newSpaceForOldPtr);
    
    //Changing prev_size to equal allocated chunk
    newSpaceForOldPtr->prev_size = newChunk.size;
    
    //Copying new struct into new chunk as final operation so any changes we make apply
    *foundFree = newChunk;

    return ret;
}

//Allows freeing of memory allocated by new_malloc
//Takes pointer returned by new_malloc
void new_free(void *ptr) {
    //Target metadata of found chunk
    chunk_n *foundChunk = (chunk_n *)(ptr-sizeof(chunk_n));

    if (!is_allocated(foundChunk)) {

        kprintf("Error finding foundChunk: %p\n", foundChunk);

        return;
    } 

    //Unmasking bits from size to signify unallocated
    foundChunk->size &= (~ALLOCATED_MASK);
    kprintf("\n\n--- Running new_free function, attempting to release %d bytes ---\n", foundChunk->size);

    //Checking if temp exists in much smaller mmap list
    chunk_n *temp = mmap_space_head;
    for (; temp; temp=temp->next) {
        if (temp==foundChunk) {
            //If present, remove from linked list holding mmapped space
            if (remove_from_list(&mmap_space_head, foundChunk)) {
                munmap(foundChunk, foundChunk->size+sizeof(chunk_n));
                kprintf("\nSUCCESSFULLY FREED MMAPPED SPACE: %x\n", foundChunk);
                return;
            }
        }
    }
    //If above failed, continue with freeing from heap space

    //Inserting into correct list within bin
    place_in_bin(foundChunk);

    //Finding next and prev to attempt coalescing
    chunk_n *nextChunk = next_chunk(foundChunk); 
    chunk_n *prevChunk = prev_chunk(foundChunk);

    kprintf("Attempting to combine: %x with %x and %x\n", foundChunk+4, prevChunk+4, nextChunk+4);

    //Coalescing and updating prev size
    update_prev_size(combine(foundChunk, nextChunk));
    update_prev_size(combine(prevChunk, foundChunk));

    kprintf("\nSUCCESSFULLY FREED %x at %x. Setting as unallocated.\n", ptr+4, foundChunk+4);
}

//Returns total free size in all bins
int total_freesize() {
    int ret = 0;
    for (int i=0; i<NUMBER_OF_BINS; i++) {
        ret += bin_list[i].bin_size;
    }
    return ret;
}

//Function searches through all free bins and prints each free list value
void print_list() {
    kprintf("\nTotal free space:%d\n", total_freesize());
    kprintf("MIN |\n");

    for (int i=0; i<NUMBER_OF_BINS; i++) {
        chunk_n *temp = bin_list[i].chunk_list_head;
        if (bin_list[i].bin_size != 0) {
            //Setting min from bin to print
            int min = bin_list[i].bin_min;
            if (bin_list[i].bin_min == -1) min=0;
            kprintf("%4d:", min);
        }
        for (; temp; temp=temp->next) {
            //Outputting formatted information about current free chunk
            if (temp->size > 0) kprintf("  %x - %d|", temp, temp->size);
            //kprintf("In bin %d\t", i);
        }
        if (bin_list[i].bin_size != 0)
            kprintf("\n");
    }
    kprintf("\n");
}