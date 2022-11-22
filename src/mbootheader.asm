; Locate OS!
; multiboot 2
section .multiboot2_header
start_:
    align 8
    ; Magic no
    dd 0xE85250D6 ; Multiboot2 magic no
    dd 0; prot i386
    dd end_ - start_
    dd 0x100000000 - (0xE85250D6 + 0 + (end_ - start_))

    ; align 8
    ; dw 3
    ; dw 1
    ; dd 12
    ; dd start_

; fb_start_: 
;             ;   Tags constitutes a buffer of structures following each other padded when necessary in order for each tag to start at 8-bytes aligned address. Tags are terminated by a tag of type ‘0’ and size ‘8’
;     align 8
;     ; Framebuffer tag goes here
;     dw 5
;     dw 1
;     dd fb_end_-fb_start_
;     dd 1024
;     dd 480
;     dd 32
; fb_end_:

    align 8
    dw 0
    dw 0
    dd 8

end_:

