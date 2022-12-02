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


    ;; Mapping page directory to itself
    mov rax, (page_table_l2-0xFFFFFFFF80000000)
    or rax, 0b11
    mov [page_table_l3 + (511*8)], rax ;; Last L3 mapped here to L2

    mov rax, (page_table_l1-0xFFFFFFFF80000000)
    or rax, 0b11
    mov [page_table_l2 + (511*8)], rax ;; Last entry of l2 maps to l1

    mov rax, (page_table_l4-0xFFFFFFFF80000000)
    or rax, 0b11
    mov [page_table_l1 + (511*8)], rax ;; Last entry of l1 holds physical address of l4

; KERNEL_OFFSET =           0xFFFFFFFF80000000;
    ;; Disable paging
    ; mov rax, cr0
    ; and rax, ~(1 << 31)
    ; mov cr0, rax

    ; mov rax, cr3

    ; mov cr3, rax
    
    ; ;; Update physical address of table

    mov rax, (page_table_l4-0xFFFFFFFF80000000)
    mov cr3, rax

    ;; Update CR3 here



    ; ; ;; Re-enable paging
    ; mov rax, cr0
    ; or rax, 1 << 31
    ; mov cr0, rax


    ; mov dword [0xb8000], 0x2f6b2f4f

    ; hlt
    ret



;; Actual page tables inherited by first process defined here
section .bss ;; block started by symbol, linker sets bytes to 0
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
    resb 4096 * 4
stk_top:

section .rodata
gdt64:
	dq 0 ; zero entry 
.code_segment: equ $ - gdt64
	dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53) ; code segment
.pointer:
	dw $  - gdt64 - 1 ; length
	dq gdt64 ; address




section .text
;; Interrupt jazz
extern exception_handler
extern panic

;; Defining macros for error stubs
%macro isr_err_stub 1
isr_stub_%+%1:
    ;; Save registers
    ;; Move interrupt number into exception handler
    mov qword rdi, %1
    call exception_handler
    iretq
%endmacro
; if writing for 64-bit, use iretq instead
%macro isr_no_err_stub 1
isr_stub_%+%1:
    mov qword rdi, %1
    call panic
    iretq
%endmacro

isr_no_err_stub 0
isr_no_err_stub 1
isr_no_err_stub 2
isr_no_err_stub 3
isr_no_err_stub 4
isr_no_err_stub 5
isr_no_err_stub 6
isr_no_err_stub 7
isr_err_stub    8
isr_no_err_stub 9
isr_err_stub    10
isr_err_stub    11
isr_err_stub    12
isr_err_stub    13
isr_err_stub    14
isr_no_err_stub 15
isr_no_err_stub 16
isr_err_stub    17
isr_no_err_stub 18
isr_no_err_stub 19
isr_no_err_stub 20
isr_no_err_stub 21
isr_no_err_stub 22
isr_no_err_stub 23
isr_no_err_stub 24
isr_no_err_stub 25
isr_no_err_stub 26
isr_no_err_stub 27
isr_no_err_stub 28
isr_no_err_stub 29
isr_err_stub    30
isr_no_err_stub 31

global isr_stub_table

;; Defining tables with stubs
isr_stub_table:
%assign i 0 
%rep    32 
    dq isr_stub_%+i
%assign i i+1 
%endrep

