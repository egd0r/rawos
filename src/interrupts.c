#define IDT_TA_InterruptGate	0b10001110 // 0x8E
#define IDT_TA_CallGate			0b10001100 // 0x8C
#define IDT_TA_TrapGate			0b10001111 // 0x8F

// Defining master/slave command and data lines
#define PIC1_COMMAND 0x20
#define PIC1_DATA	 0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA	 0xA1

#define PIC1_OFFSET  0x20 // Up to 0x1F taken by x86_64
#define PIC2_OFFSET	 0x28

//Defining values passed to PIC
#define ICW1_INIT	0x10
#define ICW1_ICW4	0x01
#define ICW4_8086	0x01
#define PIC_EOI		0x20

#define PS2_PORT	0x60

void remapPIC();
static inline void outb(uint16_t port, uint8_t val);
static inline uint8_t inb(uint16_t port);

typedef struct {
	uint16_t    isr_low;      // The lower 16 bits of the ISR's address
	uint16_t    kernel_cs;    // The GDT segment selector that the CPU will load into CS before calling the ISR
	uint8_t	    ist;          // The IST in the TSS that the CPU will load into RSP; set to zero for now
	uint8_t     attributes;   // Type and attributes; see the IDT page
	uint16_t    isr_mid;      // The higher 16 bits of the lower 32 bits of the ISR's address
	uint32_t    isr_high;     // The higher 32 bits of the ISR's address
	uint32_t    reserved;     // Set to zero
} __attribute__((packed)) idt_entry_t;

typedef struct {
	uint16_t	limit;
	uint64_t	base;
} __attribute__((packed)) idtr_t;

static idtr_t idtr;

__attribute__((aligned(0x10))) 
static idt_entry_t idt[256]; // Create an array of IDT entries; aligned for performance

//Interrupt handlers
__attribute__((interrupt));
void exception_handler(uint8_t interrupt_index) {
	printf("Recoverable interrupt %d\n", interrupt_index);
    __asm__ volatile ("cli; hlt"); // Completely hangs the computer
}


__attribute__((interrupt));
void panic(uint8_t interrupt_index) {
	__asm__ volatile ("cli");
	printf("Non recoverable interrupt %d, PANIC PANIC PANIC\n", interrupt_index);
	__asm__ volatile ("hlt");
}

__attribute__((interrupt));
void pagefault_handler() {
	__asm__ volatile ("cli");
	printf("Page fault detected.");
	while(1); // Spin
}

// No ISR! Cannot recover. ABORT.
__attribute__((interrupt));
void doublefault_handler() {
	__asm__ volatile ("cli");
	printf("Double fault detected.");
	while(1); // Spin
}

// Segment error, executing priv instructions in user mode, can point to offending instruction 
// Can be recoverered from
__attribute__((interrupt));
void gpfault_handler() {
	__asm__ volatile ("cli");
	printf("General protection fault detected.");
	while(1); // Spin
}

__attribute__((interrupt));
void kb_handler() {
	__asm__ volatile ("cli");
	uint8_t scode = inb(PS2_PORT);
	printf("%d\n", scode); 
	// Eventually spawn a new process to handle the input of keyboard keys
	// For now can decode code here and place on screen
	picEOI(0x21-PIC1_OFFSET);
	__asm__ volatile ("sti");
}

__attribute__((interrupt));
void interrupt_handler() {
	__asm__ volatile ("cli");
	printf("Interrupt detected"); // Move program counter down to skip offending instruction??
	__asm__ volatile ("sti");
}

// Pass index into IDT + pointer of subr to run
void idt_set_descriptor(uint8_t vector, void *isr, uint8_t flags) {
    idt_entry_t *descriptor = &idt[vector];

	descriptor->isr_low = (uint64_t)isr & 0xFFFF;
	descriptor->kernel_cs = 0x08; // gdt.code_segment - how to ref? CPU already knows where GDT is
	descriptor->ist = 0;
	descriptor->attributes = flags;
	descriptor->isr_mid = ((uint64_t)isr >> 16) & 0xFFFF;
    descriptor->isr_high = ((uint64_t)isr >> 32) & 0xFFFFFFFF;
    descriptor->reserved = 0;

}


extern void *isr_stub_table[];
void idt_init() {
	idtr.base = (uint64_t)&idt[0];
	idtr.limit = (uint16_t)sizeof(idt_entry_t) * 255 - 1;

	for (int i=0; i<32; i++) {
		idt_set_descriptor(i, isr_stub_table[i], IDT_TA_InterruptGate);
	}

    // idt_set_descriptor(0x08, &doublefault_handler, IDT_TA_InterruptGate);
	idt_set_descriptor(0x0D, &gpfault_handler, IDT_TA_InterruptGate);
	
	remapPIC();

	//KB stuff
	outb(PIC1_DATA, 0b11111101); // Enabling keyboard by unmasking correct line in PIC master
	outb(PIC2_DATA, 0b11111111); 
	idt_set_descriptor(0x21, &kb_handler, IDT_TA_InterruptGate); //KB corresponds to 0x20 (offset) +  0x02 (KB)

	__asm__ volatile("lidt %0" : : "m"(idtr));
	__asm__ volatile("sti");
}

// IO functions - for communicating on IO bus
void picEOI(unsigned char irq) {
	if (irq >= 8)
		outb(PIC2_COMMAND, PIC_EOI);
	outb(PIC1_COMMAND, PIC_EOI); // If IRQ came from slave, EOI must be sent to both
}

/*
	Port to comms on and value to send
*/
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
    /* There's an outb %al, $imm8  encoding, for compile-time constant port numbers that fit in 8b.  (N constraint).
     * Wider immediate constants would be truncated at assemble-time (e.g. "i" constraint).
     * The  outb  %al, %dx  encoding is the only option for all other cases.
     * %1 expands to %dx because  port  is a uint16_t.  %w1 could be used if we had the port number a wider C type */
}

/* 
	Poll port for input
*/
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ( "inb %1, %0"
                   : "=a"(ret)
                   : "Nd"(port) );
    return ret;
}

/*
	Waits for a bit, some devices are slowwwww
*/
static inline void io_wait(void) {
    outb(0x80, 0); // Unused IO port - wasted IO cycle gives other devices time to catch up
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