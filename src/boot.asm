global start
global gdt64
extern long_mode_start
extern parse

section .bootstrap
bits 32

align 4096
IDT:
    dw 0 ; len
    dd 0 ; base

start:
    cli ;; Disable interrupts - I'll be careful, promise!

    mov dword [mbootstruct], ebx

    mov esp, stk_top_boot ; Init stack pointer

    ; Switch to long mode
    call check_multiboot
    call check_cpuid
    call check_long

    ; Before long mode, set up paging
    call init_page_tables
    call enable_paging
    ; Processor now mapping virtual addresses
    ; Virtual address 0xC000 0000 = 1100 0000 0000 0000   0000 0000 0000 0000  ->  0000 0000 0000 0000   0000 0000 0000 0000

    lgdt [boot_gdt64.pointer] ; GDT is at address 
    
    ;; Jumping from compatibility mode to long mode
    jmp boot_gdt64.code_segment:long_mode_start ;; Long jump to 64 bit segment


    hlt ; Hang

test_call:
    ret

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

; Init pages and identity map first 16MB 
;; Maps same 16MB to higher address space and:
;; Maps page table to itself
init_page_tables:
    ; identity mapping phys = virt
    ; paging enabled automatically when long mode is enabled
    ; Must transform addresses since .bss thinks it's at virtual address already but we need physical addresses for this stage
    mov eax, page_table_l3_boot
    or eax, 0b11
    mov [(page_table_l4_boot)], eax ;
    ;; Higher half mapping
    mov [(page_table_l4_boot + (511*8))], eax ;; move l3 into high address space too

    mov eax, page_table_l2_boot
    or eax, 0b11
    mov [page_table_l3_boot], eax
    mov [page_table_l3_boot + (510*8)], eax ;; Move into correct index of l3

    mov ecx, 0
loop:

    mov eax, 0x00200000 ; Mapping from 2MB into memory
    mul ecx           ; Mapping 2MB * n from memory and placing in page table  
    or eax, 0b10000011 ; Modifying flags of entry


    mov [(page_table_l2_boot) + ecx * 8], eax ; 8 bytes per entry. This is important for PAE 64-bit where 0-63 are used as address

    inc ecx
    cmp ecx, 8 ; Map 8 entries providing 2MB * 8 = 16MB of mappings for the kernel initially, more than enough (hopefully lol)
    jne loop

    ;; Mapping page table to itself
    mov eax, page_table_l4_boot
    or eax, 0b11
    mov [page_table_l3_boot + (511*8)], eax
    ; mov [page_table_l3 + (1023*4)], eax
    ; mov eax, 0xFFFFFFFF
    ; mov [page_table_l3 + (1022*4)], eax

    ret

enable_paging:
    ; Disable paging

    ; Setting PAE enable bit in CR4
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; Loading cr3 with phys address of PML4
    mov eax, (page_table_l4_boot)
    mov cr3, eax

    ; Enable long mode
    mov ecx, 0xC0000080 ; Selecting EFER register
    rdmsr

    or eax, 1 << 8      ; Enabling long mode bit
    wrmsr               ; Writing to EFER register

    ; Enable paging
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ret

error:
    mov dword [0xb8000], 0x4f535f45
    mov dword [0xb8004], 0x4f3a4f52
    mov dword [0xb8008], 0x4f204f20
    mov byte [0xb800A], al
    hlt     ; Hang

section .boot_bss ;; block started by symbol, linker sets bytes to 0
align 4096
page_table_l4_boot:
    resb 4096
page_table_l3_boot:
    resb 4096
page_table_l2_boot:
    resb 4096
stk_bott_boot:
    resb 4096 * 4
stk_top_boot:

; GDT
global mbootstruct
section .boot_rodata
bits 64
boot_gdt64:
	dq 0 ; zero entry 
.code_segment: equ $ - boot_gdt64
	dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53) ; code segment
.pointer:
	dw $  - boot_gdt64 - 1 ; length
	dq boot_gdt64 ; address

mbootstruct:
    dq 0 ;; Reserved for multiboot struct information