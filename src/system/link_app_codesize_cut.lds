/*
 * Script for GNU linker.
 * Describes layout of sections, location of stack.
 */

MEMORY
{
    /*flash   (rx): ORIGIN = 0x00024C00, LENGTH = 512K*/
    flash   (rx): ORIGIN = 0x1df00, LENGTH = 512K
    ram   (rw!x): ORIGIN = 0x00400F00, LENGTH = 48K
}

OUTPUT_ARCH(arm)
OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")

ENTRY(_vector_start);

/*_vector_start = 0x00024C00; //for define CFG_DBG*/
/*_vector_start = 0x1df00;*/
SECTIONS
{
     /*. = 0x00024C00; //for define CFG_DBG*/
    . = 0x1df00;
    .vectors :
    {
        *(*.vectors)
    } > flash

    . = ALIGN(32);

   /* FAST_CALL_SEG 0x00024F00://for define CFG_DBG*/
    FAST_CALL_SEG 0x1e200:
   {
        /*Do not change the section order, for image*/
        *(sys_fiq_entry)
        . = ALIGN(32);
        *(sys_irq_entry)   
    } > flash

    . = ALIGN(8);

    .text :
    {
        *(.text)
        *(.text.*)
        KEEP(*(*.symbols))
    } > flash

    . = ALIGN(8);

    .rodata :
    {
        *(.rodata)
        *(.rodata.*)
        *(.eh_frame)
    } > flash

    .ARM.exidx :
    {
        *(.ARM.exidx*)
        *(.gnu.linkonce.armexidx.*)
    } > flash

    . = ALIGN(4);

    data_load_begin = .;

    .data : AT(data_load_begin)
    {
        *(.data .data.*)
        *(.sdata)
    } > ram

    . = ALIGN(4);

    _data_load_begin = LOADADDR(.data);
    _data_addr_begin = ADDR(.data);
    _data_addr_end   = .;

    /* global pointer in middle of data, so as much as possible is accessible */
    PROVIDE(__gp = .);

    .bss :
    {
        _bss_start = .;
        *(.bss .bss.*);
        . = ALIGN(4);
        _bss_end = .;
    } > ram

    /* This symbol defines end of code/data sections. Heap starts here. */
    PROVIDE(end = .);

    .stack :
    {
        *(.stack)
    } > ram
}
