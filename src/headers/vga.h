static void cls(void);
static void cls (void);
static void itoa (char *buf, int base, int d);
static void putchar (int c);

#define KERNEL_VIRT 0xFFFFFFFF80000000
#define VIDEO 0xb8000+KERNEL_VIRT // Using vid base+virt since we're now in higher half ;)
static volatile unsigned char *video;

#define COLUMNS 80
#define LINES 24
#define ATTRIBUTE 7
