OUTPUT_ARCH(arm)

ENTRY(_start)

romBase   = 0x00201000;
bss1Base  = 0x00102400;
etcBase   = 0x00102400;
bss2Base  = 0x00108900;
dramBase  = 0x11221000;
stackBase = 0x00200000;

MEMORY {
    bss1  : ORIGIN = bss1Base, LENGTH = 0x6500
    bss2  : ORIGIN = bss2Base, LENGTH = 0x5300
    rom   : ORIGIN = romBase, LENGTH = 0x1F000
    dram  : ORIGIN = dramBase, LENGTH = 0x6C00
    etc   : ORIGIN = etcBase, LENGTH = 0x6800
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
        *(EXCLUDE_FILE(*tz_*.o *device_apc.o *handshake_*.o *usbd.o *usbtty.o *circbuf.o *dramc_*.o *emi_init.o *memory.o *efuse_self.o *efuse_writer.o *sec_efuse.o *efuse_runtime.o) .text)
        *(EXCLUDE_FILE(*tz_*.o *device_apc.o *handshake_*.o *usbd.o *usbtty.o *circbuf.o *dramc_*.o *emi_init.o *memory.o *efuse_self.o *efuse_writer.o *sec_efuse.o *efuse_runtime.o) .text.*)
    } >rom

    .rodata ALIGN(4) : {
        *(EXCLUDE_FILE(*tz_*.o *device_apc.o *handshake_*.o *usbd.o *usbtty.o *circbuf.o *dramc_*.o *emi_init.o *memory.o *efuse_self.o *efuse_writer.o *sec_efuse.o *efuse_runtime.o) .rodata)
        *(EXCLUDE_FILE(*tz_*.o *device_apc.o *handshake_*.o *usbd.o *usbtty.o *circbuf.o *dramc_*.o *emi_init.o *memory.o *efuse_self.o *efuse_writer.o *sec_efuse.o *efuse_runtime.o) .rodata.*)
    } >rom

    .data ALIGN(4) : {
        *(EXCLUDE_FILE(*tz_*.o *device_apc.o *handshake_*.o *usbd.o *usbtty.o *circbuf.o *dramc_*.o *emi_init.o *memory.o *efuse_self.o *efuse_writer.o *sec_efuse.o *efuse_runtime.o) .data)
        *(EXCLUDE_FILE(*tz_*.o *device_apc.o *handshake_*.o *usbd.o *usbtty.o *circbuf.o *dramc_*.o *emi_init.o *memory.o *efuse_self.o *efuse_writer.o *sec_efuse.o *efuse_runtime.o) .data.*)
    } >rom

    .got ALIGN(4) : {
        *(EXCLUDE_FILE(*tz_*.o *device_apc.o *handshake_*.o *usbd.o *usbtty.o *circbuf.o *dramc_*.o *emi_init.o *memory.o *efuse_self.o *efuse_writer.o *sec_efuse.o *efuse_runtime.o) .got)
        *(EXCLUDE_FILE(*tz_*.o *device_apc.o *handshake_*.o *usbd.o *usbtty.o *circbuf.o *dramc_*.o *emi_init.o *memory.o *efuse_self.o *efuse_writer.o *sec_efuse.o *efuse_runtime.o) .got.*)
    } >rom

    __boot_end = .;

    . = dramBase;
    .loader_ext_dram ALIGN(4) : AT(ADDR(.got) + SIZEOF(.got)) {
        *dramc_*.o (.text .text.* .rodata .rodata.* .data .data.* .got .got.*)
        *emi_init.o (.text .text.* .rodata .rodata.* .data .data.* .got .got.*)
        *memory.o (.text .text.* .rodata .rodata.* .data .data.* .got .got.*)
    } >dram
    ASSERT(!((ADDR(.loader_ext_dram) + SIZEOF(.loader_ext_dram)) > 0x11227BFF), "Error: loader_ext_dram over size!")

    . = etcBase;
    .loader_ext_etc ALIGN(4) : AT(ADDR(.got) + SIZEOF(.got) + SIZEOF(.loader_ext_dram)) {
        *handshake_*.o (.text .text.* .rodata .rodata.* .data .data.* .got .got.*)
        *usbd.o (.text .text.* .rodata .rodata.* .data .data.* .got .got.*)
        *usbtty.o (.text .text.* .rodata .rodata.* .data .data.* .got .got.*)
        *circbuf.o (.text .text.* .rodata .rodata.* .data .data.* .got .got.*)
        *tz_*.o (.text .text.* .rodata .rodata.* .data .data.* .got .got.*)
        *device_apc.o (.text .text.* .rodata .rodata.* .data .data.* .got .got.*)
        *efuse_self.o (.text .text.* .rodata .rodata.* .data .data.* .got .got.*)
        *efuse_writer.o (.text .text.* .rodata .rodata.* .data .data.* .got .got.*)
        *sec_efuse.o (.text .text.* .rodata .rodata.* .data .data.* .got .got.*)
        *efuse_runtime.o (.text .text.* .rodata .rodata.* .data .data.* .got .got.*)
    } >etc
    /* loader_ext_etc will overwrite bss1, so its size could be the size of bss1. */
    /* However, we limit the size to 25 KB. */
    ASSERT(!((ADDR(.loader_ext_etc) + SIZEOF(.loader_ext_etc)) > 0x00108800), "Error: loader_ext_etc over size!")

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

        *nand.o (.gpt_sram_buf)
        *nand.o (.gpt_sram_part_meta_info)
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
