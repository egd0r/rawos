global start
extern long_mode_start

section .text
bits 32

start:
    mov esp, stk_top ; Init stack pointer

    ; Switch to long mode
    call check_multiboot
    call check_cpuid
    call check_long

    ; Before long mode, set up paging
    call init_page_tables
    call enable_paging

    lgdt [gdt64.pointer]
    jmp gdt64.code_segment:long_mode_start


    hlt ; Hang

check_multiboot:
    cmp eax, 0x36D76289 ; Compliant bootloader will store magic val here in eax
    jne .nomboot
    ret

.nomboot:
    mov al, "M"
    jmp error

; Attempt to flip CPUID in flags register
; If it remains flipped, CPUID available
check_cpuid:
    pushfd  ; Push flags to stack
    pop eax ; Pop flags to eax
    mov ecx, eax ; Make copy to compare later
    xor eax, 1 << 21 ; Flip ID bit (bit 21)
    push eax ; push eax to stack
    popfd   ; pop flags from stack
    pushfd  ; push to stack
    pop eax ; pop from stack to eax for comparison
    push ecx; push value from ecx so values remain the same as before
    popfd   ; replenishing flags reg with old values
    cmp eax, ecx ; compare prev to now
    je .no_cpuid ; equal (bit not flipped, call error)
    ret

.no_cpuid:
    mov al, "C"
    jmp error

; Support 64 bit?
check_long:
    mov eax, 0x80000000
    cpuid ; Takes eax as argument, store number > eax if ext mode not available
    cmp eax, 0x80000001
    jb .no_long

    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz .no_long

    ret

.no_long:
    mov al, "L"
    jmp error

init_page_tables:
    ; identity mapping phys = virt
    ; paging enabled automatically when long mode is enabled
    mov eax, page_table_l3
    or eax, 0b11
    mov [page_table_l4], eax

    mov eax, page_table_l2
    or eax, 0b11
    mov [page_table_l3], eax

    mov ecx, 0
loop:

    mov eax, 0x200000
    mul ecx
    or eax, 0b10000011

    mov [page_table_l2 + ecx * 8], eax

    inc ecx
    cmp ecx, 512
    jne loop

    ret

enable_paging:
    ; Disable paging

    ; Setting PAE enable bit in CR4
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; Loading cr3 with phys address of PML4
    mov eax, page_table_l4
    mov cr3, eax

    ; Enable long mode

    mov ecx, 0xC0000080
    rdmsr

    or eax, 1 << 8
    wrmsr

    ; Enable paging
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax
    

    ret

error:
    mov dword [0xb8000], 0x4f535f45
    mov dword [0xb8004], 0x4f3a4f52
    mov dword [0xb8008], 0x4f204f20
    mov dword [0xb800A], "C"
    hlt     ; Hang

section .bss
align 4096
page_table_l4:
    resb 4096
page_table_l3:
    resb 4096
page_table_l2:
    resb 4096
stk_bott:
    resb 4096 * 4
stk_top:

; GDT
section .rodata
gdt64:
	dq 0 ; zero entry
.code_segment: equ $ - gdt64
	dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53) ; code segment
.pointer:
	dw $ - gdt64 - 1 ; length
	dq gdt64 ; address
