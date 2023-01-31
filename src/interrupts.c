#include "headers/interrupts.h"
#include "headers/multitasking.h"
#include "headers/vga.h" // For prints
// #include "headers/syscalls.h"

static idtr_t idtr;

__attribute__((aligned(0x10))) 
static idt_entry_t idt[256]; // Create an array of IDT entries; aligned for performance


/*
	Time alive incremented with tick times with each PIT IRQ call
*/
uint64_t ms_since_boot = 0x00;
uint64_t frac_ms_since_boot = 0x00;

uint64_t time_between_irq_ms = 0x0A;
uint64_t time_between_irq_frac = 0x0013DF85E201BD446E9A;

void get_virt_test_i() {
	print_reg("IDTR", (uint64_t)(&idtr));
	print_reg("IDT", (uint64_t)idt);
}


void print_reg(char *name, uint64_t reg) {
	uint32_t hi = (reg & 0xFFFFFFFF00000000) >> 32;
	uint32_t lo = reg;

	printf("    %s    0x%8x:%8x\n", name, hi, lo);
}

void print_reg_state(INT_FRAME frame) {
	printf("DUMP:\n");
	print_reg("RSP", frame.rsp);
	print_reg("R15", frame.r15);
	print_reg("R14", frame.r14);
	print_reg("R13", frame.r13);
	print_reg("R12", frame.r12);
	print_reg("RBX", frame.rbx);
	print_reg("RBP", frame.rbp);
	print_reg("RIP", frame.rip);
	print_reg("CS ", frame.cs);
}
                  
void * task_switch_int(INT_FRAME *frame) {
	// Pass to scheduler, get new context
	// Locking and unlocking implementations written in for DPC(?)
	TASK_LL *new_task = schedule(frame);
	if (new_task == NULL) return frame;

	// CPU_STATE *new_state = new_task->state;

	// // If stack has not moved, process is starting, must initialise stack
	// // Otherwise, context has already been saved on the stack.
	// if (new_state->rsp == &(new_task->stack[STACK_SIZE-1])) {
	// 	// Placing correct values on stack - checked and is filling correctly
	// 	INT_FRAME *new_frame = ((INT_FRAME *)(new_state->rsp-sizeof(INT_FRAME)));

	// 	new_frame->vector = 0x20;
	// 	new_frame->rsp = &new_frame->rsp; // new_frame+sizeof(INT_FRAME); 
	// 	new_frame->r15 = new_state->r15;
	// 	new_frame->r14 = new_state->r14;
	// 	new_frame->r13 = new_state->r13;
	// 	new_frame->r12 = new_state->r12;
	// 	new_frame->rbx = new_state->rbx;
	// 	new_frame->rbp = new_state->rbp;
	// 	new_frame->rip = new_state->rip;
	// 	new_frame->eflags = new_state->eflags;
	// 	new_frame->cs  = new_state->cs; // Keep as start of stack if this pointer is replacing stack at end...

	// 	return new_frame;
	// }
	
	return new_task->stack;
}

//Interrupt handlers
int counter = 0;
void * exception_handler(INT_FRAME frame) {
	// printf("Recoverable interrupt 0x%2x\n", frame.vector);
	// print_reg_state(frame);
	void *ret = &frame;
	if (frame.vector >= 0x20 && frame.vector < 0x30) {
		// interrupt 20h corresponds to PIT
		// Switch contexts 
		ms_since_boot += time_between_irq_ms;
		uint64_t temp = frac_ms_since_boot + time_between_irq_frac;
		if (frac_ms_since_boot > temp) ms_since_boot++; // Overflow
		frac_ms_since_boot += temp;

		ret = task_switch_int(&frame);

		picEOI(frame.vector-PIC1_OFFSET);
	} else if (frame.vector == 0x0D) {
		printf("General protection fault -_-\n");
		print_reg_state(frame);
		__asm__ volatile ("hlt");
	} else if (frame.vector == 0x80) {
		// syscall_handler(&frame);
	} else {
		cls();
		printf("Interrupted %d\n", frame.vector);
		print_reg_state(frame);
		__asm__ volatile ("hlt");
	}

	return ret;
}


__attribute__((interrupt));
void panic(INT_FRAME frame) {
	__asm__ volatile ("cli");
	printf("Non recoverable interrupt 0x%2x, PANIC PANIC PANIC\n", frame.vector);
	print_reg_state(frame);

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

	for (int i=0; i<33; i++) {
		idt_set_descriptor(i, isr_stub_table[i], IDT_TA_InterruptGate);
	}

    // idt_set_descriptor(0x08, &doublefault_handler, IDT_TA_InterruptGate);
	// idt_set_descriptor(0x0D, &gpfault_handler, IDT_TA_InterruptGate);
	
	remapPIC();
	
	// PIT
	set_pit_freq(11932);
	//KB stuff
	outb(PIC1_DATA, 0b11111100); // Enabling keyboard by unmasking correct line in PIC master
	outb(PIC2_DATA, 0b11111111); 
	// idt_set_descriptor(0x20, &pit, IDT_TA_InterruptGate); //Programmable interrupt timer, for scheduling tasks
	idt_set_descriptor(0x21, &kb_handler, IDT_TA_InterruptGate); //KB corresponds to 0x20 (offset) +  0x02 (KB)

	__asm__ volatile("lidt %0" : : "m"(idtr));
	CLI();
}

void activate_interrupts() {
	__asm__ volatile("sti");
}

/*
	Waits for a bit, some devices are slowwwww
*/
static inline void io_wait(void) {
    outb(0x80, 0); // Unused IO port - wasted IO cycle gives other devices time to catch up
}

// IO functions - for communicating on IO bus
void picEOI(unsigned char irq) {
	if (irq >= 8)
		outb(PIC2_COMMAND, PIC_EOI);
	outb(PIC1_COMMAND, PIC_EOI); // If IRQ came from slave, EOI must be sent to both
}

void set_pit_freq(uint16_t reload_count) {
	CLI();
	outb(PIT_CHNL_0, reload_count & 0xFF);
	io_wait();
	outb(PIT_CHNL_0, (reload_count & 0xFF00) >> 8);
	STI();
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