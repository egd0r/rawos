#include <types.h>

// Defining master/slave command and data lines
#define PIC1_COMMAND 0x20
#define PIC1_DATA	 0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA	 0xA1

#define PIC1_OFFSET  0x20 // Up to 0x1F taken by x86_64
#define PIC2_OFFSET	 0x28

#define PIT_CHNL_0	 0x40
// #define PIT_FREQ	 0x

//Defining values passed to PIC
#define ICW1_INIT	0x10
#define ICW1_ICW4	0x01
#define ICW4_8086	0x01
#define PIC_EOI		0x20

#define PS2_PORT	0x60

void outb(uint16_t port, uint8_t val);
uint8_t inb(uint16_t port);
void io_wait(void);
void picEOI(unsigned char irq);
void remapPIC();