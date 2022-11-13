ENTRY(start)
OUTPUT_FORMAT(elf64-x86-64)

SECTIONS
{
    . = 1M; /* Load 1MB in to enable pagin and shift to long mode */

    boot : 
    {
        *(.multiboot2_header)
        *(.bootstrap)
        *(.rodata*)
    }

    .text : AT(ADDR(.text))
    {
        _code = .;
        *(.text)
        . = ALIGN(4096);
    }

   .data : AT(ADDR(.data))
   {
        _data = .;
        *(.data)
        . = ALIGN(4096);
   }

   .eh_frame : AT(ADDR(.eh_frame))
   {
       _ehframe = .;
       *(.eh_frame)
        . = ALIGN(4096);
   }

   .bss : AT(ADDR(.bss))
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

