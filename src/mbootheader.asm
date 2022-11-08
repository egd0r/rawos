; Locate OS!
; multiboot 2
section .multiboot2_header
start_:
    ; Magic no
    dd 0xE85250D6 ; Multiboot2 magic no
    dd 0; prot i386
    dd end_ - start_

    dd 0x100000000 - (0xE85250D6 + 0 + (end_ - start_))

    dw 0
    dw 0
    dd 8

end_:

