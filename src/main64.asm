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

    
    xor rdi, rdi ;; Clearing RDI, is this necessary?
    mov qword rdi, [mbootstruct] ; Passing struct to C function
    call kmain

    hlt


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

