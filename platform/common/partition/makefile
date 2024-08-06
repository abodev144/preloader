###################################################################
# flag
###################################################################

MOD_SRC  =
MOD_OBJ  = $(PRELOADER_OUT)/obj

###################################################################
# source files
# - add your file here !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
###################################################################

MOD_SRC  := part_common.c

ifeq ("$(CFG_BOOT_DEV)", "BOOTDEV_NAND")
MOD_SRC	 += part_nand.c
endif

ifeq ("$(CFG_BOOT_DEV)", "BOOTDEV_SDMMC")
MOD_SRC	 += part_emmc.c
endif

ifeq ("$(CFG_BOOT_DEV)", "BOOTDEV_UFS")
MOD_SRC	 += part_ufs.c
endif

###################################################################
# target
###################################################################

include $(COMMON_FILE_MK)

