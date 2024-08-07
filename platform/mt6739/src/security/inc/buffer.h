/*
* This software/firmware and related documentation ("MediaTek Software") are
* protected under relevant copyright laws. The information contained herein
* is confidential and proprietary to MediaTek Inc. and/or its licensors.
* Without the prior written permission of MediaTek inc. and/or its licensors,
* any reproduction, modification, use or disclosure of MediaTek Software,
* and information contained herein, in whole or in part, shall be strictly prohibited.
*/
/* MediaTek Inc. (C) 2011. All rights reserved.
*
* BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
* THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
* RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
* AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
* NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
* SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
* SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
* THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
* THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
* CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
* SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
* STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
* CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
* AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
* OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
* MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*
* The following software/firmware and/or related documentation ("MediaTek Software")
* have been modified by MediaTek Inc. All revisions are subject to any receiver's
* applicable license agreements with MediaTek Inc.
*/

/******************************************************************************
*
* Filename:
* ---------
*     buffer.h
*/
#ifndef BUFFER_ADDR_H
#define BUFFER_ADDR_H

#include "platform.h"
#include "dram_buffer.h"

/************************************/
/*preloader download DA use DRAM*/
#define DA_RAM_ADDR                 (CFG_DA_RAM_ADDR)
#define DA_RAM_LENGTH               (0x40000)
/*preloader validate DA use DRAM*/
#define DA_RAM_RELOCATE_ADDR        (CFG_DA_RAM_ADDR + DA_RAM_LENGTH)
#define DA_RAM_RELOCATE_LENGTH      (DA_RAM_LENGTH)

#define  SEC_SECRO_BUF    g_dram_buf->sec_secro_buf 
#define  SEC_WORKING_BUF  g_dram_buf->sec_working_buf
#define  SEC_LIB_HEAP_BUF g_dram_buf->sec_lib_heap_buf
#define  SEC_IMG_BUF      g_dram_buf->sec_img_buf

/******************************************************************************
 * on-chip SRAM range
 ******************************************************************************/
#define SEC_SRAM_WORKING_BUF        0x00100400
#define SEC_SRAM_IMG_BUF            0x00100500
#define SEC_SRAM_HEAP_BUF           0x00101400
#define SEC_SRAM_IMG_LENGTH         0x0F00
#define SEC_SRAM_HEAP_LENGTH        0x1000

#endif

