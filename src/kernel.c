void printf (const char *format, ...);

#include "headers/types.h"
#include "headers/vga.h"
#include "headers/multiboot2.h"
#include "headers/interrupts.h"
#include "headers/stdarg.h"
#include "headers/paging.h"

#include "headers/memory.h"

//Try and parse tag without framebuffer enabled
// Getting struct as argument from rdi // 32 bit ptr => 0x 00 00 00 00
int kmain(unsigned long mbr_addr) {
    // Initialise IDT

    asm __volatile__("mov %rsp, [stk_top]"); // Recreating stack in kmain

    idt_init();

    // Map pages? Already got 16MB identity mapped
    // Parse mb struct
    cls();
    struct multiboot_tag_mmap *ret = init_memory_map(mbr_addr);

    // Allocate at 0
    uint64_t ptr = kalloc_physical(1);
    // Free at 0
    kfree_physical((void *)ptr);

    // Getting 1 byte (1 page) from physical memory
    printf("%x\n", ptr);

    // unmap_page(0x0); //Unmapping first 16MB 

    // Unmapping lower half CHANGE STACK PTR INSIDE KMAIN

    // uint64_t *virt_addr = p_alloc(PAGE_DIR_VIRT, 1); // Trying to allocate first free page
    // printf("Address allocated: %x\n", virt_addr);



    // printf("\n");
  

    printf("OK!\n");

    //Trying to deref page directory
    uint64_t *pd = PAGE_DIR_VIRT;
    get_physaddr(PAGE_DIR_VIRT);


    *((int *)0xb8900) = mbr_addr;   // print address
    // *((int *)0xb8900) = 0x00000000;




    printf("SPINNING!\n"); 
    
    // printf("\nTesting page fault: %d\n", 14);
    // int *testpf = 0x80085405835;
    // *testpf = 5; // lel
    
    
    while (1) {
      // printf("hello");
    }; //Spin on hang
    // Spawn init process
    
    return 0;
}

void
cls (void)
{
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