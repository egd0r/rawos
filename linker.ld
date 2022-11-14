ENTRY(start)
OUTPUT_FORMAT(elf64-x86-64)

SECTIONS
{
    . = 1M; /* Load 1MB in to enable pagin and shift to long mode */

    boot : 
    {
        *(.multiboot2_header)
        *(.bootstrap)
        *(.bs64)
        *(.rodata*)
        *(.bss)
    }
    

    /* Only jump to these addresses once paging is enabled and these virtual addresses are mapped  */
    . += 0xFFFF800000000000; /* Linking straight to higher half */

    .text : AT(ADDR(.text) - 0xFFFF800000000000)
    {
        _code = .;
        *(.text)
        *(.ro64*)
        . = ALIGN(4096);
    }

   .data : AT(ADDR(.data) - 0xFFFF800000000000)
   {
        _data = .;
        *(.data)
        . = ALIGN(4096);
   }

   .eh_frame : AT(ADDR(.eh_frame) - 0xFFFF800000000000)
   {
       _ehframe = .;
       *(.eh_frame)
        . = ALIGN(4096);
   }

   .bss : AT(ADDR(.bss) - 0xFFFF800000000000)
   {
       _bss = .;
        *(.bss)
       /*
        * You usually need to include generated COMMON symbols
        * under kernel BSS section or use gcc's -fno-common
        */

        *(COMMON)
       . = ALIGN(4096);
   }

   _end = .;

   /DISCARD/ :
   {
        *(.comment)
   }
}

