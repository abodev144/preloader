OUTPUT_ARCH(arm)

ENTRY(_start)

romBase   = 0x00201000;
bss1Base  = 0x00102400;
bss2Base  = 0x00108C00;
stackBase = 0x0010D000;

MEMORY {
    bss1  : ORIGIN = bss1Base, LENGTH = 0x6800
    bss2  : ORIGIN = bss2Base, LENGTH = 0x4400
    rom   : ORIGIN = romBase, LENGTH = 0x1F000
    stack : ORIGIN = stackBase, LENGTH = 0xC00
}

SECTIONS {
    . = romBase;
    .start ALIGN(4) : {
        *(.text.start)
    } >rom

    . = romBase + 0x01FC;
    .rom_info ALIGN(4) : {
        *(.data.rom_info)
    } >rom

    .text ALIGN(4) : {
        *(.text)
        *(.text.*)
    } >rom

    .rodata ALIGN(4) : {
        *(.rodata)
        *(.rodata.*)
    } >rom

    .data ALIGN(4) : {
        *(.data)
        *(.data.*)
    } >rom

    .got ALIGN(4) : {
        *(.got)
        *(.got.*)
    } >rom

    __boot_end = .;

    . = bss2Base;
    .bss2 ALIGN(16) : {
        _bss2_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        /* make _bss1_end as 4 bytes alignment */
        . = ALIGN(4);
        _bss2_end = .;
    } >bss2

    . = bss1Base;
    .bss1 ALIGN(16) : {
        _bss1_start = .;
        *print.o (.log_sram_buf)
        *efi.o (.gpt_sram_buf)
        *efi.o (.gpt_sram_crc32_table)
        *efi.o (.gpt_sram_part_info)
        *efi.o (.gpt_sram_part_meta_info)
        /* make _bss1_end as 4 bytes alignment */
        . = ALIGN(4);
        _bss1_end = .;
    } >bss1

    . = stackBase;
    .stack ALIGN(4) : {
        _stack_start = .;
        *(.stack)
        _stack_end = .;
    } >stack

    .secbuf ALIGN(4) : {
        _secbuf_start = .;
        *(.secbuf)
        _secbuf_end = .;
    } >bss1
}
