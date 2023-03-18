#pragma once
#define IDT_TA_InterruptGate	0b10001110 // 0x8E
#include "types.h"

#define IDT_TA_CallGate			0b10001100 // 0x8C
#define IDT_TA_TrapGate			0b10001111 // 0x8F

#define TEN_MS 11932
#define PIT_RELOAD TEN_MS*3



void remapPIC();
void outb(uint16_t port, uint8_t val);
uint8_t inb(uint16_t port);

typedef struct {
	uint16_t    isr_low;      
	uint16_t    kernel_cs;    
	uint8_t	    ist;          
	uint8_t     attributes;   
	uint16_t    isr_mid;      
	uint32_t    isr_high;     
	uint32_t    reserved;     
} __attribute__((packed)) idt_entry_t;

typedef struct {
	uint16_t	limit;
	uint64_t	base;
} __attribute__((packed)) idtr_t;


typedef struct {
	uint64_t vector;
	uint64_t cr3;
	uint64_t r15;
	uint64_t r14;
	uint64_t r13;
	uint64_t r12;
	uint64_t rax;
	uint64_t rcx;
	uint64_t rdx;
	uint64_t rbx;
	uint64_t rbp;
	uint64_t rdi;
	uint64_t rsi;
	uint64_t err;
	uint64_t rip;
	uint64_t cs;
	uint64_t eflags;
	uint64_t rsp;
} __attribute__((packed)) INT_FRAME;

//Need to make available for assembly routines
void * exception_handler(INT_FRAME * frame, uint64_t arg);
void panic(INT_FRAME frame);
// void pagefault_handler();
// void doublefault_handler();
// void gpfault_handler();
// void kb_handler();
// void interrupt_handler();

void idt_set_descriptor(uint8_t vector, void *isr, uint8_t flags);
void idt_init();

void activate_interrupts();