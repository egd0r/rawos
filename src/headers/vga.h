void cls (void);
void itoa (char *buf, int base, int d);
void putchar (int c);
void printf(const char *format, ...);
void kprintf(const char *format, ...);

#include <multitasking.h>

#define KERNEL_VIRT 0xFFFFFFFF80000000
#define VIDEO_ACTUAL 0xb8000+KERNEL_VIRT // Using vid base+virt since we're now in higher half ;)
#define VIDEO 0xb8000+KERNEL_VIRT // Using vid base+virt since we're now in higher half ;)
#define PROC_VIDEO HEAP_START
static volatile unsigned char *video = (unsigned char *)VIDEO;

#define COLUMNS 80
#define LINES 24

#define VIDEO_END VIDEO + ((COLUMNS + LINES * LINES) * 2)

#define VIDEO_SIZE VIDEO_END - VIDEO

#define ATT_BLACK      0x0
#define ATT_BLUE       0x1
#define ATT_GREEN      0x2
#define ATT_CYAN       0x3
#define ATT_RED        0x4
#define ATT_MAGENTA    0x5
#define ATT_BROWN      0x6
#define ATT_LT_GREY    0x7
#define ATT_DK_GREY    0x8
#define ATT_LT_BLUE    0x9
#define ATT_LT_GREEN   0xA
#define ATT_LT_CYAN    0xB
#define ATT_LT_RED     0xC
#define ATT_LT_MAGENTA 0xD
#define ATT_LT_BROWN   0xE
#define ATT_WHITE      0xF

static int xpos = 0;
static int ypos = 0;