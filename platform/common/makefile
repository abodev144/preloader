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
	addr_trans.c \
	div0.c \
	stdlib.c \
	sec_hrid.c \
	string.c \
	error.c

ifeq ("$(CFG_PROFILING_SUPPORT)", "yes")
MOD_SRC	 += profiling.c
else
MOD_SRC	 += profiling_dummy.c
endif

ifeq ("$(CFG_PLAT_DBG_INFO_LIB)", "1")
	MOD_SRC  += plat_dbg_info.c
endif

ifeq ("$(MTK_AB_OTA_UPDATER)", "yes")
MOD_SRC += ab_bootcheck.c
endif
###################################################################
# target
###################################################################
include $(COMMON_FILE_MK)

SUBDIRS += partition
SUBDIRS += storage
SUBDIRS += loader
SUBDIRS += bootctrl
SUBDIRS += emimpu
SUBDIRS += mblock
SUBDIRS += watchdog

include $(COMMON_DIR_MK)

