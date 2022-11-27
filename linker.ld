ENTRY(start)
OUTPUT_FORMAT(elf64-x86-64)

KERNEL_OFFSET = 0xFFFFFFFF80000000;
KERNEL_START = 0x10000;

SECTIONS
{
    . = KERNEL_START; /* Load 1MB in to enable pagin and shift to long mode */

    boot : AT(ADDR(boot)) /* At 1MB */  
    {
        KEEP(*(.multiboot2_header))
        *(.bootstrap)
        *(.bss)
        *(.rodata)
    }

    . += KERNEL_OFFSET;

    .text : AT(ADDR(.text) - KERNEL_OFFSET)
    {
        _code = .;
        *(.text)
        *(.eh_frame)
        . = ALIGN(4096);
    }

   .bss : AT(ADDR(.bss) - KERNEL_OFFSET)
   {
       _bss = .;
       *(.bss*)
        *(.rodata*)

       /*
        * You usually need to include generated COMMON symbols
        * under kernel BSS section or use gcc's -fno-common
        */

        *(COMMON)
       . = ALIGN(4096);
   }

   _end = .;
}
