#include <types.h>
#include <vga.h>
#include <multiboot2.h>
#include <interrupts.h>
#include <paging.h>
#include <memory.h>
#include <multitasking.h>
//For testing
#include <ata.h>
#include <io.h>

/*
    Can force scheduler by calling interrupt 0x20 = 32 in stub table which corresponds to timer interrupt

*/
// Can align for memory safety when copying to user page table
__attribute__((aligned(0x1000))) 
void taskA() {
    // for (int i=0; i<100; i++) {
    //     i--;
    // }
    int *x = (int *)new_malloc(sizeof(int)*3);
    x[0] = 5;
    while (1)
        printf("A");
}

void taskB() {
    // for (int i=0; i<100; i++) {
    //     i--;
    // }
    while(1) {
        printf("B");
    }
}

void taskC() {
    // for (int i=0; i<100; i++) {
    //     i--;
    // }
    while (1) {
        printf("C");
    }
}

// extern void switch_task();

/*
    TODO:
        - ASM routine to switch tasks without having to trigger an interrupt so tasks can be pre-empted
        - Find a way to share kernel structures across page tables in processes (shared memory)
        - Message passing between processes
        - InitRD which is loaded through GRUB, containing user processes - WCS don't need this can start all in kmain

*/

extern uint64_t RODATA_START;
extern void syscal_test(int syscall_num);

int kmain(unsigned long mbr_addr) {
    heap_current = heap_start;
    // Initialise IDT
    asm __volatile__("mov %rsp, [stk_top]"); // Recreating stack in kmain

    idt_init();

    // Map pages? Already got 16MB identity mapped
    // Parse mb struct
    cls();
    memset(BITMAP_VIRTUAL, ~0, 0x10000); //Sets bitmap to entirely allocated

    // Initialises memory map and finds initrd
    init_memory_map(mbr_addr);

    extern uint64_t initrd_start;
    // init_tar_fs(initrd_start);



    syscal_test(42);

    // Allocate at 0
    uint64_t ptr = kalloc_physical(1);
    ptr = kalloc_physical(1);
    ptr = kalloc_physical(1);
    ptr = kalloc_physical(1);
    // Free at 0
    kfree_physical((void *)ptr);

    // Getting 1 byte (1 page) from physical memory
    printf("%x\n", ptr);

    // unmap_page(0x0); //Unmapping first 16MB 

    // Unmapping lower half CHANGE STACK PTR INSIDE KMAIN

    // uint64_t *virt_addr = p_alloc(PAGE_DIR_VIRT, 1); // Trying to allocate first free page
    // int *virt_addr = page_alloc(PAGE_DIR_VIRT, PT_LVL4, 1); // Trying to allocate first free page
    // // uint64_t *virt_addr = page_alloc(KERNEL_LVL2_MAP, PT_LVL2, 1) | KERNEL_OFFSET; // Trying to allocate first free page
    // *virt_addr = 100;
    
    // uint64_t *virt_addr_new = sbrk(1); // Trying to allocate first free page
    // *virt_addr_new = 1024;

    // uint64_t *sbrk_eg = sbrk(0);

    int *arr = new_malloc(sizeof(int)*5);
    arr[1] = 5;

    uint64_t main_cr3 = (uint64_t)get_pagetable_entry(0xffffff7fbfdfe000) & ~0xFFF;
    uint64_t taskA_phys = (uint64_t)get_pagetable_entry(&taskA);

    print_reg("RODATA", &RODATA_START);
    print_reg("L4 physical", main_cr3);
    print_reg("TA physical", taskA_phys);

    // get_virt_test_i();


    // for (int i=0; i<5; i++) {
    //     arr[i] = i;
    // }

    // new_free(arr);

    printf("OK!\n");

    //Trying to deref page directory - PF int 0x0E
    // *((uint64_t *)0x222222222222) = 453;


    *((int *)0xb8900) = mbr_addr;   // print address
    // *((int *)0xb8900) = 0x00000000;

    printf("SPINNING!\n"); 
    
    // printf("\nTesting page fault: %d\n", 14);
    create_task(0x00);
    // create_task(&taskA);
    // create_task(&taskB);
    // create_task(&taskC);
    activate_interrupts(); // sti
    // CLI();

    // Saves state on stack and selects a new process to run (sets current_item)
    // schedule(current_item->stack);
    // Takes stack of item to switch to and performs context switch with ret
    // -> Examine stack and swaps incase of error
    // switch_task(current_item->stack);


    cls();
    extern uint64_t ms_since_boot;
    while(ms_since_boot != 10000);
    // printf("%c ", getch());
    // printf("%c ", getch());
    // printf("%c ", getch());
    // printf("%c ", getch());
    // printf("%c ", getch());

    while (1) {
    //   printf("kernel task");
        char ch = getch();
        if (ch != -1) printf("%c ", ch);
    }; //Spin on hang
    // Spawn init process
    
    return 0;
}
