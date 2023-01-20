#pragma once
#define IDT_TA_InterruptGate	0b10001110 // 0x8E
#include "types.h"

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


typedef struct {
	uint64_t vector;
	uint64_t r15;
	uint64_t r14;
	uint64_t r13;
	uint64_t r12;
	uint64_t rbx;
	uint64_t rbp;
	uint64_t rip;
	uint64_t cs;
	uint64_t eflags;
	uint64_t rsp;
} INT_FRAME;

//Need to make available for assembly routines
void * exception_handler(INT_FRAME frame);
void panic(INT_FRAME frame);
// void pagefault_handler();
// void doublefault_handler();
// void gpfault_handler();
// void kb_handler();
// void interrupt_handler();

void idt_set_descriptor(uint8_t vector, void *isr, uint8_t flags);
void idt_init();

void activate_interrupts();