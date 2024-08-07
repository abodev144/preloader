/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#ifndef PARTITION_H
#define PARTITION_H

#include "typedefs.h"
#include "blkdev.h"
#include "mmc_core.h"
#include "ufs_aio_core.h"

#define PART_HEADER_DEFAULT_ADDR    (0xFFFFFFFF)
#define LOAD_ADDR_MODE_BACKWARD     (0x00000000)
#define PART_MAGIC          0x58881688
#define EXT_MAGIC           0x58891689
#define PT_MAGIC            0x58901690

typedef union
{
    struct
    {
        unsigned int magic;     /* partition magic */
        unsigned int dsize;     /* partition data size */
        char name[32];          /* partition name */
        unsigned int maddr;     /* partition memory address */
        unsigned int mode;      /* memory addressing mode */
        /* extension */
        unsigned int ext_magic; /* always EXT_MAGIC */
        unsigned int hdr_sz;
        unsigned int hdr_ver;
        unsigned int img_type;
        unsigned int img_list_end;
        unsigned int align_sz;
        unsigned int dsize_extend;
        unsigned int maddr_extend;
    } info;
    unsigned char data[512];
} part_hdr_t;

typedef struct {
	unsigned int magic;     /* magic */
	unsigned int hdr_size;  /* data size */
	unsigned int img_size;  /* img size */
	unsigned int align;     /* align */
	unsigned int id;        /* image id */
	unsigned int addr;      /* memory addr */
} ptimg_hdr_t;

struct part_info_t {
    u64 addr;
    u8 active;
};

#if (CFG_BOOT_DEV == BOOTDEV_SDMMC) || (CFG_BOOT_DEV == BOOTDEV_UFS)

#define PART_META_INFO_NAMELEN  64
#define PART_META_INFO_UUIDLEN  16
#define PART_ATTR_LEGACY_BIOS_BOOTABLE  (0x00000004UL) /* bit2 = active bit for OTA */

struct part_meta_info {
    u8 name[PART_META_INFO_NAMELEN];
    u8 uuid[PART_META_INFO_UUIDLEN];
};

typedef struct {
    unsigned long start_sect;
    unsigned long nr_sects;
    unsigned long part_attr;
    unsigned int part_id;
    struct part_meta_info *info;
} part_t;

#else

enum partition_type
{
    TYPE_LOW,
    TYPE_FULL,
    TYPE_SLC,
    TYPE_TLC,
};

typedef struct
{
    unsigned char *name;        /* partition name */
    unsigned long startblk;     /* partition start blk */
    unsigned long long size;    /* partition size */
    unsigned long blks;         /* partition blks */
    unsigned long flags;        /* partition flags */
    unsigned int part_id;
    enum partition_type type;
} part_t;

#endif

extern int part_init(void);
extern part_t *part_get(char *name);
extern int part_load(blkdev_t *bdev, part_t *part, u32 *addr, u32 offset, u32 *size);
extern void part_dump(void);

extern part_t *get_part(char *name);
extern int part_load_raw_part(blkdev_t *bdev, part_t *part, u32 *addr, u32 offset, u32 *size);
extern unsigned long mt_part_get_active_bit(part_t *part);
extern unsigned long mt_part_get_start_addr(part_t *part);
extern unsigned long mt_part_get_size(part_t *part);
extern unsigned int mt_part_get_part_id(part_t *part);
extern int if_equal_img_name(blkdev_t *bdev, part_t *part, u32 offset, const char* img_name);
extern int mt_get_part_info_by_name(const char *name, struct part_info_t *part_info);
extern u64 get_part_addr(const char *name);

#endif /* PARTITION_H */
