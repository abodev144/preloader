OUTPUT_ARCH(arm)

ENTRY(_start)

romBase = 0x00201000;
ramBase = 0x00102180;

MEMORY {
    ram : ORIGIN = ramBase, LENGTH = 0xBA80
    rom : ORIGIN = romBase, LENGTH = 0x1F000
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

    . = ramBase;
    .bss ALIGN(16) : {
        _bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        /* make _bss_end as 4 bytes alignment */
        . = ALIGN(4);
        _bss_end = .;
    } >ram

    .secbuf ALIGN(4) : {
        _secbuf_start = .;
        *(.secbuf)
        _secbuf_end = .;
    } >ram
}
