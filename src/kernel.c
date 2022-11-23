void printf (const char *format, ...);

#include "types.h"
#include "multiboot2.h"
#include "interrupts.c"

#define VIDEO 0xb8000
static volatile unsigned char *video;

#define COLUMNS 80
#define LINES 24
#define ATTRIBUTE 7

static void cls(void);
static void cls (void);
static void itoa (char *buf, int base, int d);
static void putchar (int c);

static int xpos = 0;
static int ypos = 0;

// extern void idt_init();


//Try and parse tag without framebuffer enabled
// Getting struct as argument from rdi // 32 bit ptr => 0x 00 00 00 00
int kmain(unsigned long mbr_addr) {
    // Initialise IDT
    // idt_init();

    idt_init();
    // Map pages? Already got 16MB identity mapped
    // Parse mb struct

    cls();

    // printf("\n");

   

    *((int *)0xb8000) = 0x2e6b2f4f; // Ok
    printf("OK!\n");

    *((int *)0xb8900) = mbr_addr;   // print address
    // *((int *)0xb8900) = 0x00000000;

    uint32_t size; // information struct is 8 bytes aligned, each field is u32 
    size = *((uint32_t *)mbr_addr); // First 8 bytes of MBR 

    struct multiboot_tag *tag;
    tag = (struct multiboot_tag *)(mbr_addr + 8); // u32+u32 = 64 = 8 bytes for next tag

    uint32_t tagType = tag->type;
    uint32_t tagSize = tag->size;

    struct multiboot_tag_framebuffer_common *fb;

    for (; tag->type != MULTIBOOT_TAG_TYPE_END; tag += 8) {
      if (tag->type == MULTIBOOT_TAG_TYPE_BASIC_MEMINFO) {
        struct multiboot_tag_basic_meminfo *basicInfo = tag;
        uint32_t mem_lower = basicInfo->mem_lower;
        uint32_t mem_upper = basicInfo->mem_upper;
      }
      if (tag->type == MULTIBOOT_TAG_TYPE_BOOTDEV) {
        struct multiboot_tag_bootdev *bootDevice = tag;
        uint32_t biosdev = bootDevice->biosdev;
        uint32_t part = bootDevice->part;
        uint32_t subPart = bootDevice->slice;
      }
      if (tag->type == MULTIBOOT_TAG_TYPE_FRAMEBUFFER) {
        fb = tag;
        uint64_t fb_phys = fb->framebuffer_addr;
      }
      if (tag->type == MULTIBOOT_HEADER_TAG_INFORMATION_REQUEST) {
        struct multiboot_header_tag_information_request *infoReq = tag;
        uint32_t *types = infoReq->requests;
      }
    }


    printf("SPINNING!\n"); 
    
    // printf("\nTesting page fault:\n");
    // int *testpf = 0x80085405835;
    // *testpf = 5; // lel
    
    
    while (1) {
      // printf("hello");
    }; //Spin on hang
    // Spawn init process
    
    return 0;
}

static void
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
static void
itoa (char *buf, int base, int d)
{
  char *p = buf;
  char *p1, *p2;
  unsigned long ud = d;
  int divisor = 10;
  
  /*  If %d is specified and D is minus, put ‘-’ in the head. */
  if (base == 'd' && d < 0)
    {
      *p++ = '-';
      buf++;
      ud = -d;
    }
  else if (base == 'x')
    divisor = 16;

  /*  Divide UD by DIVISOR until UD == 0. */
  do
    {
      int remainder = ud % divisor;
      
      *p++ = (remainder < 10) ? remainder + '0' : remainder + 'a' - 10;
    }
  while (ud /= divisor);

  /*  Terminate BUF. */
  *p = 0;
  
  /*  Reverse BUF. */
  p1 = buf;
  p2 = p - 1;
  while (p1 < p2)
    {
      char tmp = *p1;
      *p1 = *p2;
      *p2 = tmp;
      p1++;
      p2--;
    }
}

/*  Put the character C on the screen. */
static void
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

/*  Format a string and print it on the screen, just like the libc
   function printf. */
void
printf (const char *format, ...)
{
  char **arg = (char **) &format;
  int c;
  char buf[20];

  arg++;
  
  while ((c = *format++) != 0)
    {
      if (c != '%')
        putchar (c);
      else
        {
          char *p, *p2;
          int pad0 = 0, pad = 0;
          
          c = *format++;
          if (c == '0')
            {
              pad0 = 1;
              c = *format++;
            }

          if (c >= '0' && c <= '9')
            {
              pad = c - '0';
              c = *format++;
            }

          switch (c)
            {
            case 'd':
            case 'u':
            case 'x':
              itoa (buf, c, *((int *) arg++));
              p = buf;
              goto string;
              break;

            case 's':
              p = *arg++;
              if (! p)
                p = "(null)";

            string:
              for (p2 = p; *p2; p2++);
              for (; p2 < p + pad; p2++)
                putchar (pad0 ? '0' : ' ');
              while (*p)
                putchar (*p++);
              break;

            default:
              putchar (*((int *) arg++));
              break;
            }
        }
    }
}