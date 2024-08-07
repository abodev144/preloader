###################################################################
# flag
###################################################################

MOD_SRC  = 
MOD_OBJ  = $(PRELOADER_OUT)/obj

###################################################################
# source files
# - add your file here !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
###################################################################

MOD_SRC  := \
	blkdev.c \
	download.c \
	eabi_uldivmod.s \
	eabi_idiv.s \
	eabi.s \
	handshake_uart.c \
	handshake_usb.c \
	main.c \
	print.c \
	dram_buffer.c

ifeq ("$(CFG_BOOT_DEV)", "BOOTDEV_NAND")
	MOD_SRC += part.c
else
	MOD_SRC += partition.c
endif
###################################################################
# target
###################################################################

include $(COMMON_FILE_MK)
