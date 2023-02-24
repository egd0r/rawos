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

	kprintf("    %s    0x%8x:%8x\n", name, hi, lo);
}

void print_reg_state(INT_FRAME frame) {
	kprintf("DUMP:\n");
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

	if ((page_ptr[l1_index] & PRESENT) != 0) kprintf("error in allocate here");
	else page_ptr[l1_index] = (uint64_t)KALLOC_PHYS() | PRESENT | RW;

	flush_tlb();
}



extern uint64_t page_table_l4; // Kernel data
void switch_screen(int PID) {

	// if (PID == current_display->PID) return;
	// // Selecting current process
	// // Return if current process is being displayed already
	// // Get task corresponding to PID passed
	// load_cr3((uint64_t)(&page_table_l4)&0xFFFFF); 
	// TASK_LL *task_new = TASK(PID);
	// if (task_new != NULL && ((task_new->flags & DISPLAY_TRUE) != DISPLAY_TRUE)) {
	// 	// Accessing kernel structures, changing task displayed
	// 	current_display->flags &= (~DISPLAY_TRUE);
	// 	current_display = task_new;
	// 	current_display->flags |= DISPLAY_TRUE;
	// 	// Switching to new process display
	// 	load_cr3(current_display->cr3); 
	// 	// Copy current display to video out
	// 	memcpy((uint8_t *)HEAP_START, (uint8_t *)VIDEO_ACTUAL, 80*24*2);//80*24*2); // 2 bytes per char
	// 	kprintf("SWITCHED to %d!", PID);
	// }
	// // Load current task again before return
	// load_cr3(current_item->cr3);
}

int kbd_us [128] =
{
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',   
  '\t', /* <-- Tab */
  'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',     
    0, /* <-- control key */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',  0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0,
  '*',
    0xA13,  /* Alt */
  ' ',  /* Space bar */
    0,  /* Caps lock */
    0,  /* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,  /* < ... F10 */
    0,  /* 69 - Num lock*/
    0,  /* Scroll Lock */
    0,  /* Home key */
    0,  /* Up Arrow */
    0,  /* Page Up */
  '-',
    0,  /* Left Arrow */
    0,
    0,  /* Right Arrow */
  '+',
    0,  /* 79 - End key*/
    0,  /* Down Arrow */
    0,  /* Page Down */
    0,  /* Insert Key */
    0,  /* Delete Key */
    0,   0,   0,
    0,  /* F11 Key */
    0,  /* F12 Key */
    0,  /* All other keys are undefined */
};

extern IN_STREAM stream;
extern char in_;
int alt_pressed = 0;
void kb_handler() {
	uint8_t scode = inb(PS2_PORT);
	// kprintf("%x ", scode);
	// If key PRESSED
	if (kbd_us[scode] == 0xA13) {
		alt_pressed = 1;
		// kprintf("ALT pressed!");
	} else if (scode == 184) {
		alt_pressed = 0;
		// kprintf("ALT released!");
	} else if (alt_pressed == 1 && scode < 0x81) {
		if (scode == 11) {
			switch_screen(0);
		} else {
			switch_screen(scode-1); // -2 to get the PID of process to switch to
		}
	} else if (scode < 0x81) {
		// printf("%c ", kbd_us[scode]);
		// IN_STREAM stream = current_display->stream;
		// stream.position = (stream.position+1)%100;
		// stream.buffer[stream.position] = kbd_us[scode];

		// load_cr3((uint64_t)current_display->cr3); 
		// putchar(kbd_us[scode], current_display);
		putchar_current(kbd_us[scode]);
		// load_cr3(current_item->cr3);

		// Backspace
		// Call vga.rem
		// remchar()

		// in_ = kbd_us[scode];
	} 
	// else {
	// 	kprintf("Unrecognised scancode: %d\n", scode);
	// }
	// For now can decode code here and place on screen
	picEOI(0x21-PIC1_OFFSET);
}

__attribute__((interrupt));
void interrupt_handler() {
	__asm__ volatile ("cli");
	kprintf("Interrupt detected"); // Move program counter down to skip offending instruction??
	__asm__ volatile ("sti");
}

//Interrupt handlers
extern void syscall_stub();
int counter = 0;
void * exception_handler(INT_FRAME * frame, uint64_t arg) {
	// Switching to interrupt stack

	// kprintf("Recoverable interrupt 0x%2x\n", frame.vector);
	void *ret = frame;
	if (frame->vector == 0x20) {
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
		kprintf("General protection fault -_-\n");
		print_reg_state(*frame);
		__asm__ volatile ("hlt");
	} else if (frame->vector == 0x0E) {
		allocate_here(arg);
	} else if (frame->vector == 0x21) {
		// Caused TF -> GPF
		// if (frame->cr3 != ((uint64_t)(&page_table_l4)&0xFFFFF))
		// 	load_cr3((uint64_t)(&page_table_l4)&0xFFFFF); // Need interrupt stack
		kb_handler();
		// load_cr3(frame->cr3); // Changing to new process address space where stack is defined
	} else if (frame->vector == 0x80) {
		syscall_handler(*frame);
	} else {
		cls();
		kprintf("Interrupted %d\n", frame->vector);
		print_reg_state(*frame);
		__asm__ volatile ("hlt");
	}

	return ret;
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
	// set_pit_freq(65536);
	//KB stuff
	outb(PIC1_DATA, 0b11111100); // Enabling keyboard by unmasking correct line in PIC master
	outb(PIC2_DATA, 0b11111111); 
	// idt_set_descriptor(0x20, &pit, IDT_TA_InterruptGate); //Programmable interrupt timer, for scheduling tasks
	// idt_set_descriptor(0x21, &kb_handler, IDT_TA_InterruptGate); //KB corresponds to 0x20 (offset) +  0x02 (KB)

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
		kprintf("No device\n");
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
		kprintf("This device is not ATA.\n");
		return;
	}

	//command
	outb(port+7, 0xEC);

	status = inb(port+7);
	if (status == 0x00) {
		kprintf("No device\n");
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
		kprintf("ERROR %d\n", status);
		return;
	}

	// data is ready to read

	for (uint16_t i=0; i<256; i++) {
		uint16_t data = inb(port);
		char *foo = "  \0";
		foo[1] = (data >> 8) & 0x00FF;
		foo[2] = (data & 0x00FF);
		kprintf(foo);
	}

}

