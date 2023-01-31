void printf (const char *format, ...);

#include "headers/types.h"
#include "headers/vga.h"
#include "headers/multiboot2.h"
#include "headers/interrupts.h"
#include "headers/stdarg.h"
#include "headers/paging.h"
#include "headers/memory.h"
#include "headers/multitasking.h"

/*
    Can force scheduler by calling interrupt 0x20 = 32 in stub table which corresponds to timer interrupt

*/
// Can align for memory safety when copying to user page table
__attribute__((aligned(0x1000))) 
void taskA() {
    // for (int i=0; i<100; i++) {
    //     i--;
    // }
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
int kmain(unsigned long mbr_addr) {
    // Initialise IDT
    asm __volatile__("mov %rsp, [stk_top]"); // Recreating stack in kmain

    idt_init();

    // Map pages? Already got 16MB identity mapped
    // Parse mb struct
    cls();
    memset(BITMAP_VIRTUAL, ~0, 0x10000); //Sets bitmap to entirely allocated
    struct multiboot_tag_mmap *ret = init_memory_map(mbr_addr);

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
    uint64_t main_cr3 = (uint64_t)get_pagetable_entry(0xffffff7fbfdfe000) & ~0xFFF;
    uint64_t taskA_phys = (uint64_t)get_pagetable_entry(&taskA);

    print_reg("L4 physical", main_cr3);
    print_reg("TA physical", taskA_phys);

    get_virt_test_i();


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
    // int *testpf = 0x80085405835;
    // *testpf = 5; // lel

    // CAUSES 0x06 and TRIPLE FAULT OCCAISONALLY?
    // create_task(HERE);
    create_task(0x00); // Init task - can move this to init_multitasking or something?
    create_task(&taskA);
    create_task(&taskB);
    create_task(&taskC);
    activate_interrupts(); // sti
    // CLI();

    // Saves state on stack and selects a new process to run (sets current_item)
    // schedule(*((INT_FRAME *)current_item->stack));
    // Takes stack of item to switch to and performs context switch with ret
    // -> Examine stack and swaps incase of error
    // switch_task(current_item->stack);

    while (1) {
    //   printf("kernel task");
    }; //Spin on hang
    // Spawn init process
    
    return 0;
}

void
cls (void)
{
  xpos = 0; ypos = 0;
  int i;

  video = (unsigned char *) VIDEO;
  
  for (i = 0; i < COLUMNS * LINES * 2; i++)
    *(video + i) = 0xFFFF;

}

/*  Convert the integer D to a string and save the string in BUF. If
   BASE is equal to ’d’, interpret that D is decimal, and if BASE is
   equal to ’x’, interpret that D is hexadecimal. */
/*  Put the character C on the screen. */
void
putchar (int c)
{
  if (c == '\n' || c == '\r')
    {
    newline:
      xpos = 0;
      ypos++;
      if (ypos >= LINES)
        ypos = 0;
      return;
    }

  *(video + (xpos + ypos * COLUMNS) * 2) = c & 0xFF;
  *(video + (xpos + ypos * COLUMNS) * 2 + 1) = ATTRIBUTE;

  xpos++;
  if (xpos >= COLUMNS)
    goto newline;
}

char *_itoa(int num, int base, char *buffer) {
    char rep[16] = "0123456789ABCDEF";
    char *ptr = &buffer[49];
    *ptr = '\0';

    do {
        *--ptr = rep[num%base];
        num /= base;
    } while( num != 0 );

    return ptr;
}

int is_digit(char let) {
    return let >= 48 && let <= 57; 
}

int _atoi(char let) {
    if (let >= 48 && let <= 57) return let-48;
    else return -1;
}

int str_len(char *string) {
    int ret = 0;
    for (; *string != '\0'; string++)
        ret++;
    return ret;
}

void pad(char paddingChar, int length) {
    if (length <= 0) return;
    for (int i=0; i<length; i++)
        putchar(paddingChar);
}

void printf(const char *format, ...) {

    va_list arg;
    va_start(arg, format);

    char *string;

    for (string=format; *string != '\0'; string++) {
        if ( *string == '%' ) {
            string++;

            int padding = 0;
            for (; is_digit(*string); string++) {
                if (padding != 0)
                    padding *= 10;
                padding += _atoi(*string);
            }

            char retstr[50];
            int dec = 0;
            int base = 0;
            char *str;

            char paddingChar;
            if ( *string == 'b' || *string == 'd' || *string == 'o' || *string == 'x' ) {
                paddingChar = '0';
                dec = va_arg(arg, int);
            } 

            switch ( *string ) {
                case 'c':
                    putchar(dec);
                    break;
                case 'd':
                    base = 10;
                    break;
                case 'o':
                    base = 8;
                    break;
                case 'b':
                    base = 2;
                    break;
                case 's':
                    paddingChar = ' ';
                    str = va_arg(arg, char *);
                    break;
                case 'x':
                    base = 16;
                    break;
                default:
                    putchar(*string);
            }

            if (base != 0) {
                str = _itoa(dec, base, retstr);
            }

            for (; padding > str_len(str); padding--)
                putchar(paddingChar);

            for (; *str != '\0'; str++)
                putchar(*str);

            string++;
        }

        putchar(*string);
    }
}