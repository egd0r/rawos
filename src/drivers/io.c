#include <io.h>
#include <multitasking.h>

// IN_STREAM stream = { .accessed = 0, .position = -1 };
int in_ = -1;

char getch() {

	// IN_STREAM stream = current_display->stream;
	IN_STREAM stream;
	if (stream.position == -1) {
		return -1;
	} else {
		// stream.accessed = 1;
		// int curr_pos = stream.position;
		// stream.position--;
		// int ret = stream.buffer[curr_pos];
		// stream.accessed = 0;
		int ret = stream.buffer[(stream.position)--];
		return ret;
	} 
}


/*
	Port to comms on and value to send
*/
void outb(uint16_t port, uint8_t val) {
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
    /* There's an outb %al, $imm8  encoding, for compile-time constant port numbers that fit in 8b.  (N constraint).
     * Wider immediate constants would be truncated at assemble-time (e.g. "i" constraint).
     * The  outb  %al, %dx  encoding is the only option for all other cases.
     * %1 expands to %dx because  port  is a uint16_t.  %w1 could be used if we had the port number a wider C type */
}

/* 
	Poll port for input
*/
uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ( "inb %1, %0"
                   : "=a"(ret)
                   : "Nd"(port) );
    return ret;
}

// Remapping PIC
/*
	Interrupts produced by PIC can't intefere with OS interrupts. 
	By default, PIC interrupts from 0x00 to 0x0F - there are two PICs, one master and one slave
*/
void remapPIC() {
	uint8_t a1, a2;

	a1 = inb(PIC1_DATA);
	io_wait();
	a2 = inb(PIC2_DATA);
	io_wait();

	//Init master
	outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
	io_wait();
	outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
	io_wait();

	//PICs need to know offsets
	outb(PIC1_DATA, PIC1_OFFSET);
	io_wait();
	outb(PIC2_DATA, PIC2_OFFSET);
	io_wait();

	//How do PICs correspond to eachother? Don't understand this bit
	outb(PIC1_DATA, 4);
	io_wait();
	outb(PIC2_DATA, 2);
	io_wait();

	//8086 mode
	outb(PIC1_DATA, ICW4_8086);
	io_wait();
	outb(PIC2_DATA, ICW4_8086);
	io_wait();

	//Restore bitmasks saved at beginning
	outb(PIC1_DATA, a1);
	io_wait();
	outb(PIC2_DATA, a2);
	io_wait();
}

/*
	Waits for a bit, some devices are slowwwww
*/
void io_wait(void) {
    outb(0x80, 0); // Unused IO port - wasted IO cycle gives other devices time to catch up
}

// IO functions - for communicating on IO bus
void picEOI(unsigned char irq) {
	if (irq >= 8)
		outb(PIC2_COMMAND, PIC_EOI);
	outb(PIC1_COMMAND, PIC_EOI); // If IRQ came from slave, EOI must be sent to both
}

