ENTRY(start)

SECTIONS
{
    . = 1M; /* Starts 1M in */

    .boot :
    {
        KEEP(*(.multiboot2_header))
    }

    .text : /* Followed by CPU instructions */
    {
        *(.text)
    }
}