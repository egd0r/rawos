#include <interrupts.h>
#include <multitasking.h>
#include <vga.h> // For prints
#include <paging.h>
#include <memory.h>
#include <io.h>

#define TEN_MS 11932
#define PIT_RELOAD TEN_MS*6*3

static idtr_t idtr;

__attribute__((aligned(0x10))) 
static idt_entry_t idt[256] __attribute__((section(".rodata"))); // Create an array of IDT entries; aligned for performance


/*
	Time alive incremented with tick times with each PIT IRQ call
*/
uint64_t ms_since_boot = 0x00;
uint64_t frac_ms_since_boot = 0x00;

uint64_t time_between_irq_ms = 60;
// uint64_t time_between_irq_frac = 0x0013DF85E201BD446E9A; // 10.xx ms
uint64_t time_between_irq_frac = 0x003C6AEA59A58DC3A282; // 60.xx ms

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

extern char in_;
int alt_pressed = 0;
extern int swap_screens(int new_screen_id);
void kb_handler() {
	uint8_t scode = inb(PS2_PORT);

	if (kbd_us[scode] == 0xA13) {
		alt_pressed = 1;
	} else if (scode == 184) {
		alt_pressed = 0;
	} else if (alt_pressed == 1 && scode < 0x81) {
		load_cr3((uint64_t)(&page_table_l4)&0xFFFFF); 
		if (scode == 11) {
			swap_screens(1);
		} else {
			swap_screens(scode-1); // -2 to get the PID of process to switch to
		}
		load_cr3(current_item->cr3);
	} else if (scode < 0x81) {
        IN_STREAM *stream = &(current_screen->conts[current_screen->selected_cont].stream);

		stream->position = (stream->position+1)%10;
		stream->buffer[stream->position] = kbd_us[scode];
	} 

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
	current_item->stack = frame;
	// kprintf("Recoverable interrupt 0x%2x\n", frame.vector);
	void *ret = frame;
	if (frame->vector == 0x20) {

		
		// interrupt 20h corresponds to PIT
		// Switch contexts 
		ms_since_boot += time_between_irq_ms;
		uint64_t temp = frac_ms_since_boot + time_between_irq_frac;
		if (frac_ms_since_boot > temp) ms_since_boot++; // Overflow
		frac_ms_since_boot += temp;

		current_item->proc_time += time_between_irq_ms;

		if (frame->cr3 != ((uint64_t)(&page_table_l4)&0xFFFFF))
			load_cr3((uint64_t)(&page_table_l4)&0xFFFFF); // Need interrupt stack

		TASK_LL *new_task = task_switch_int(frame);

		// Checking blocked queue and unblocking processes as needed
		TASK_LL *prev = NULL;
		for (TASK_LL *temp_blkd = blocked_start; prev != blocked_end && blocked_start != NULL; temp_blkd=temp_blkd->next) {
			temp_blkd->wake_after_ms -= time_between_irq_ms;
			if (temp_blkd->wake_after_ms <= 0) {
				ready_end->next = temp_blkd;
				ready_end = temp_blkd;
				temp_blkd->task_state = READY;

				if (prev == NULL) { 
					// temp_blkd == blocked_start is true
					if (blocked_start->next != blocked_start) blocked_start = blocked_start->next;
					else {
						blocked_start = NULL;
						blocked_end = NULL;
					}
				} else {
					prev->next = temp_blkd->next;
				}

				temp_blkd->next = NULL;
			}

			prev = temp_blkd;
		}



		picEOI(frame->vector-PIC1_OFFSET);


		load_cr3(new_task->cr3); // Changing to new process address space where stack is defined
		ret = (void *)(new_task->stack);
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
		syscall_handler(&ret);
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
	set_pit_freq(PIT_RELOAD);
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

