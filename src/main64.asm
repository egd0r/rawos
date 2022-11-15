global long_mode_start
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

    
    push rbx
    call kmain

    hlt

