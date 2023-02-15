#include <interrupts.h>
#include <multitasking.h>
#include <vga.h> // For prints
#include <paging.h>
#include <memory.h>
#include <io.h>

static idtr_t idtr;

__attribute__((aligned(0x10))) 
static idt_entry_t idt[256] __attribute__((section(".rodata"))); // Create an array of IDT entries; aligned for performance


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

extern load_kernel_segment();
extern load_user_segment();
extern void load_cr3(uint64_t pt);
extern void load_cr3_test(uint64_t pt);
TASK_LL * task_switch_int(INT_FRAME *frame) {
	// Pass to scheduler, get new context
	TASK_LL *new_task = schedule(frame);
	if (new_task == NULL) return current_item;

	if (new_task->PID == 0) {
		// load_kernel_segment();
	} else {
		// load_user_segment();
	}

	// load_cr3(new_task->cr3);

	return new_task;
}

extern void flush_tlb();
// Allocates a physical page at virt_addr
void allocate_here(uint64_t virt_addr) {
	virt_addr >>= 12;
	int l1_index = virt_addr & 0x1FF;
	int l2_index = (virt_addr >> 9) & 0x1FF;
	virt_addr >>= 9;
	int l3_index = (virt_addr >> 9) & 0x1FF;
	virt_addr >>= 9;
	int l4_index = (virt_addr >> 9) & 0x1FF;
	
    uint64_t *page_ptr = PAGE_DIR_VIRT; //Used for indexing P4 directly

	if ((page_ptr[l4_index] & PRESENT) == 0) {
		page_ptr[l4_index] = (uint64_t)KALLOC_PHYS() | PRESENT | RW;
	}

    page_ptr = ((((uint64_t)page_ptr << 9) | l4_index << 12) & PT_LVL3) | ((uint64_t)page_ptr & ~PT_LVL3);

	if ((page_ptr[l3_index] & PRESENT) == 0) {
		page_ptr[l3_index] = (uint64_t)KALLOC_PHYS() | PRESENT | RW;
	}

	page_ptr = ((((uint64_t)page_ptr << 9) | l3_index << 12) & PT_LVL2) | ((uint64_t)page_ptr & ~PT_LVL2);
	
	if ((page_ptr[l2_index] & PRESENT) == 0) {
		page_ptr[l2_index] = (uint64_t)KALLOC_PHYS() | PRESENT | RW;
	}

	page_ptr = ((((uint64_t)page_ptr << 9) | l2_index << 12) & PT_LVL1) | ((uint64_t)page_ptr & ~PT_LVL1);

	if ((page_ptr[l1_index] & PRESENT) != 0) printf("error");
	else page_ptr[l1_index] = (uint64_t)KALLOC_PHYS() | PRESENT | RW;

	flush_tlb();
}

//Interrupt handlers
extern void syscall_stub();
int counter = 0;
extern uint64_t page_table_l4; // Kernel data
void * exception_handler(INT_FRAME * frame, uint64_t arg) {
	// Switching to interrupt stack

	// printf("Recoverable interrupt 0x%2x\n", frame.vector);
	// print_reg_state(frame);
	void *ret = frame;
	if (frame->vector >= 0x20 && frame->vector < 0x30) {
		// interrupt 20h corresponds to PIT
		// Switch contexts 
		ms_since_boot += time_between_irq_ms;
		uint64_t temp = frac_ms_since_boot + time_between_irq_frac;
		if (frac_ms_since_boot > temp) ms_since_boot++; // Overflow
		frac_ms_since_boot += temp;

		if (frame->cr3 != ((uint64_t)(&page_table_l4)&0xFFFFF))
			load_cr3((uint64_t)(&page_table_l4)&0xFFFFF); // Need interrupt stack

		TASK_LL *new_task = task_switch_int(frame);

		picEOI(frame->vector-PIC1_OFFSET);

		ret = (void *)(new_task->stack);

		load_cr3(new_task->cr3); // Changing to new process address space where stack is defined
	} else if (frame->vector == 0x0D) {
		printf("General protection fault -_-\n");
		print_reg_state(*frame);
		__asm__ volatile ("hlt");
	} else if (frame->vector == 0x0E) {
		// if (arg > heap_current || arg < heap_start) return;
		// printf("Page fault");
		// __asm__ volatile ("hlt");
		/*
			Pre-reqs for interrupt based physical memory allocation:
			-> Add kernel structs to specific page and copy on create
			-> 
		*/

		allocate_here(arg);

		// Memory access is re-run
	} else if (frame->vector == 0x80) {
		syscall_handler(*frame);
	} else {
		cls();
		printf("Interrupted %d\n", frame->vector);
		print_reg_state(*frame);
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
	printf("%x\n", scode); 
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

void set_pit_freq(uint16_t reload_count) {
	CLI();
	outb(PIT_CHNL_0, reload_count & 0xFF);
	io_wait();
	outb(PIT_CHNL_0, (reload_count & 0xFF00) >> 8);
	STI();
}

extern void *isr_stub_table[];
void idt_init() {
	idtr.base = (uint64_t)&idt[0];
	idtr.limit = (uint16_t)sizeof(idt_entry_t) * 255 - 1;

	for (int i=0; i<256; i++) {
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



#include <ata.h>

void detect(uint16_t port, int master) {
    // Indentifying drive

    // Trying to talk to master
	outb(port+6, master == 1 ? 0xA0 : 0xB0);
	outb(port+0x206, 0);

	outb(port+6, 0xA0);

	uint8_t status = inb(port+7);
	if (status == 0xFF) {
		printf("No device\n");
		return;
	}

	outb(port+6, master == 1 ? 0xA0 : 0xB0);

	// Sector count port
	outb(port+2, 0);

	// //lba low
	// outb(port+3, 0);
	// //lba mid
	// outb(port+4, 0);
	// //lba hi
	// outb(port+5, 0); 	

	// Checking if these are ATA
	int lbalow = inb(port+3);
	int lbamid = inb(port+4);
	int lbahi  = inb(port+5);

	if (lbalow || lbamid || lbahi) {
		printf("This device is not ATA.\n");
		return;
	}

	//command
	outb(port+7, 0xEC);

	status = inb(port+7);
	if (status == 0x00) {
		printf("No device\n");
		return;
	}


	// Wait until device is ready
	// while(((status & 0x80) == 0x80)
	// 		&& ((status & 0x01) != 0x01))
	// 		status = inb(port+7);

	for (int i=0; i<15; i++) {
		status = inb(port+7);
	}

	if (status & 0x01) {
		printf("ERROR %d\n", status);
		return;
	}

	// data is ready to read

	for (uint16_t i=0; i<256; i++) {
		uint16_t data = inb(port);
		char *foo = "  \0";
		foo[1] = (data >> 8) & 0x00FF;
		foo[2] = (data & 0x00FF);
		printf(foo);
	}

}

