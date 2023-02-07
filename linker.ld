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
        *(.boot_bss)
        *(.boot_rodata)
    }

    . += KERNEL_OFFSET;

    /* Forget about boot stuff when in here */
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
       . = ALIGN(4096);
       RODATA_START = .;
        *(.rodata*)

       /*
        * You usually need to include generated COMMON symbols
        * under kernel BSS section or use gcc's -fno-common
        */

        *(COMMON)
       . = ALIGN(4096);
   }

   KERNEL_END = . - KERNEL_OFFSET; /* Need to make note of what physical memory 
                                is taken up by the kernel to know where bitmap can go */ 
}
