void cls (void);
void itoa (char *buf, int base, int d);
void putchar (int c);
void printf(const char *format, ...);


#define KERNEL_VIRT 0xFFFFFFFF80000000
#define VIDEO 0xb8000+KERNEL_VIRT // Using vid base+virt since we're now in higher half ;)
static volatile unsigned char *video;

#define COLUMNS 80
#define LINES 24
#define ATTRIBUTE 7

static int xpos = 0;
static int ypos = 0;