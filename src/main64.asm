global long_mode_start
extern mbootstruct
extern kmain

section .bootstrap
bits 64 ; in 64 bit mode

long_mode_start:
    ; Load 0
    mov ax, 0
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov rsp, stk_top; Init stack pointer

    mov qword [0xb8228], rbx
    
    ; Writing to video memory
    ; mov dword [0xb8000], 0x2f6b2f4f

    ;; Modify page tables inside .bss ; map kernel to 
    ;; 

    ; Jump into kernel entry here
    cli                           ; Clear the interrupt flag.
    mov ax, 0            ; Set the A-register to the data descriptor.
    mov ds, ax                    ; Set the data segment to the A-register.
    mov es, ax                    ; Set the extra segment to the A-register.
    mov fs, ax                    ; Set the F-segment to the A-register.
    mov gs, ax                    ; Set the G-segment to the A-register.
    mov ss, ax                    ; Set the stack segment to the A-register.
    mov edi, 0xB8000              ; Set the destination index to 0xB8000.
    mov rax, 0x1F201F201F201F20   ; Set the A-register to 0x1F201F201F201F20.
    mov ecx, 500                  ; Set the C-register to 500.
    rep stosq                     ; Clear the screen.

    call MapPages
    mov dword [0xb8000], 0x2f6b2f4f

    ;; Higher half page tables are now initialised

    mov qword rdi, [mbootstruct] ; Passing struct to C function
    call qword kmain

    hlt

MapPages:
    mov rax, (page_table_l3-0xFFFFFFFF80000000)
    or rax, 0b11

    mov [(page_table_l4)], rax
    mov [(page_table_l4 + (511*8))], rax ;; move l3 into high address space too

    mov rax, (page_table_l2-0xFFFFFFFF80000000)
    or rax, 0b11
    mov [page_table_l3], rax
    mov [page_table_l3 + (510*8)], rax

    mov rcx, 0
.loop:
    mov rax, 0x00200000 ; Mapping from 2MB into memory
    mul rcx           ; Mapping 2MB * n from memory and placing in page table  
    or rax, 0b10000011 ; Modifying flags of entry

    mov [(page_table_l2) + (rcx * 8)], rax ; 8 bytes per entry. This is important for PAE 64-bit where 0-63 are used as address

    inc rcx
    cmp rcx, 9 ; Map 8 entries providing 2MB * 8 = 16MB of mappings for the kernel initially, more than enough (hopefully lol)
               ; +1 for 2MB physical bitmap
    jne .loop

    mov rax, (page_table_l4-0xFFFFFFFF80000000)
    or rax, 0b11
    mov [page_table_l4 + (510*8)], rax ;; Last entry of l1 holds physical address of l4

    mov rax, (page_table_l4-0xFFFFFFFF80000000)
    mov cr3, rax

    ret



;; Actual page tables inherited by first process defined here
section .bss ;; block started by symbol, linker sets bytes to 0
global stk_top
global stk_bott
align 4096
page_table_l4:
    resb 4096
page_table_l3:
    resb 4096
page_table_l2:
    resb 4096
page_table_l1:
    resb 4096
stk_bott:
    resb 4096 * 16 ;; 32 * 4k = 128k
stk_top:


;; Global descriptor table
;; 
section .rodata
global gdt64
global gdtr_pt
gdt64:
	dq 0 ; zero entry 
.code_segment: equ $ - gdt64
	dq (1 << 43) | (0 << 44) | (1 << 47) | (1 << 53) ; code segment for kernel
.pointer:
	dw $  - gdt64 - 1 ; length
	dq gdt64 ; address


gdtr_pt: ;; Pointer to GDT used after long jump
    dw 0
    dd 0


section .text
;; Interrupt jazz
extern exception_handler
extern panic

extern test_state

;; Pushing register state to stack to avoid bad things!
%macro pushreg 0
push rbp
push rbx
push r12
push r13
push r14
push r15
; mov rax, cr3
; push rax
%endmacro

;; Popping register state from stack
%macro popreg 0
; pop rax
; mov cr3, rax
pop r15
pop r14
pop r13
pop r12
pop rbx
pop rbp
%endmacro

;; Defining macros for error stubs
%macro isr_err_stub 1
isr_stub_%+%1:
    ;; Save registers
    ;; Move interrupt number into exception handler
    pushreg
    push qword %1
    ; push qword 0xEE
    mov rdi, [rsp+0x0D] ;; Load address of stack + 7 for start of struct + 4 for OG
    call exception_handler
    mov rsp, rax ;; Set new stack
    pop rdi ;; Popping vector into rdi will get overwritten
    popreg
    ; call test_state
    iretq
%endmacro

%assign i 0 
%rep    255
    isr_err_stub i
%assign i i+1 
%endrep

global isr_stub_table

;; Defining tables with stubs
isr_stub_table:
%assign i 0 
%rep    255
    dq isr_stub_%+i
%assign i i+1 
%endrep

extern lock_scheduler
extern unlock_scheduler
; global switch_task

; ;; For sleep: save state in current -> add to blocked -> switch to next task
; ;; Assumes state has been saved
; ;; Input: rax -> stack pointer holding INT_FRAME of state
; switch_task:
;     call lock_scheduler
;     mov rsp, rax
;     pop rdi ;; Popping vector
;     popreg  ;; Popping rest
;     pop r10 ;; Popping RIP into r10
;     call unlock_scheduler
;     pop cs ;; Popping code segment
;     pop rax ;; Popping rflags ???
;     ; mov rax, cr3
;     ; push rax
;     ; pop rax
;     ; mov cr3, rax
;     push r10 ;; Pushing RIP to stack for return
;     ret