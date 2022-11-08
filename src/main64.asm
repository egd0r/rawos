global long_mode_start

section .text
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
    mov dword [0xb8000], 0x2f6b2f4f


    hlt

