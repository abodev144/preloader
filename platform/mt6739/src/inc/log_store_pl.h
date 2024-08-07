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

#ifndef __LOG_STORE_H__
#define __LOG_STORE_H__

#include <typedefs.h>
#include "ram_console.h"

#define SRAM_HEADER_SIG (0xabcd1234)
#define DRAM_HEADER_SIG (0x5678ef90)
#define LOG_STORE_SIG (0xcdab3412)
#define SRAM_LOG_ADDR (RAM_CONSOLE_SRAM_ADDR + 0xD00)
#define SRAM_LOG_SIZE   0x100   //256bytes
#define MAX_DRAM_COUNT  2
#define LOG_STORE_SIZE 0x40000  // DRAM buff 256KB
#define LOG_STORE_SIZE_MIRROR 0x40000   // Mirror dram buff 256KB

//  log flag
#define BUFF_VALID      0x01
#define CAN_FREE        0x02
#define NEED_SAVE_TO_EMMC   0x04
#define RING_BUFF       0x08    // ring buf, if buf_full, buf_point is the start of the buf, else buf_point is the buf end, other buf is not used
#define BUFF_FULL       0x10    // buf is full
#define ARRAY_BUFF      0X20    // array buf type, buf_point is the used buf end
#define BUFF_ALLOC_ERROR    0X40
#define BUFF_ERROR      0x80
#define BUFF_NOT_READY  0x100
#define BUFF_READY      0x200
#define BUFF_EARLY_PRINTK   0x400   // pl or lk can printk the early printk information to uart cable

struct mirror_log {
	u32 start_addr;
	u32 size;
};

struct mirror_log_header {
	u32 sig;
	u32 count;//mirror log save times
	struct mirror_log mlog[2];//twice pllk log header
};

struct pl_lk_log {
	u32 sig;            // default 0xabcd1234
	u32 buff_size;      // total buf size
	u32 off_pl; // pl offset, sizeof(struct pl_lk_log)
	u32 sz_pl;  // preloader size
	u32 pl_flag;    // pl log flag
	u32 off_lk; // lk offset, sizeof((struct pl_lk_log) + sz_pl
	u32 sz_lk;  // lk log size
	u32 lk_flag;    // lk log flag
};

typedef enum {
	LOG_PL_LK = 0x0,        // Preloader and lk log buff
	LOG_XXX = 0x1
} BUFF_TYPE;

/* total 80 char size. u32 20 */
struct dram_buf_header {
	u32 sig;
	u32 flag;
	u32 buf_addr;
	u32 buf_size;
	u32 buf_offsize;
	u32 buf_point;
	u32 mirror_header_addr;
	u32 klog_addr;
	u32 klog_size;
	u32 atf_log_addr;
	u32 atf_log_len;
	u32 reserve2[9];
};

/* total 256 char size */
struct sram_log_header {
	u32 sig;
	u32 reboot_count;
	u32 save_to_emmc;
	u32 reserve[1]; // reserve 4 char size
	struct dram_buf_header dram_buf[MAX_DRAM_COUNT]; // 160 char size(2 * 80)
	struct pl_lk_log dram_curlog_header; // 32 char size(8 * 4)
	struct mirror_log_header dram_mlog_header; // 48 char size(12 * 4)
};

void pl_log_store(char c);
void store_switch_to_dram(void);

#endif // #ifndef __RAM_CONSOLE_H__

