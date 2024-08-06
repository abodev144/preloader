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
 * MediaTek Inc. (C) 2016. All rights reserved.
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

/*
 * setup block correctly, we should hander both 4GB mode and
 * non-4GB mode.
 */
#include "mblock.h"
#include "platform.h"
#include "dram_buffer.h"
#include <pal_log.h>
#define bootarg g_dram_buf->bootarg

void setup_mblock_info(mblock_info_t *mblock_info, dram_info_t *orig_dram_info,
                       mem_desc_t *lca_reserved_mem)
{
	int i;
	u64 max_dram_size = -1; /* MAX value */
	u64 size = 0;
	u64 total_dram_size = 0;

	for (i = 0; i < orig_dram_info->rank_num; i++) {
		total_dram_size +=
		    orig_dram_info->rank_info[i].size;
	}
#ifdef CUSTOM_CONFIG_MAX_DRAM_SIZE
	max_dram_size = CUSTOM_CONFIG_MAX_DRAM_SIZE;
	pal_log_info("CUSTOM_CONFIG_MAX_DRAM_SIZE: 0x%llx\n", max_dram_size);
#endif
	lca_reserved_mem->start = lca_reserved_mem->size = 0;
	memset(mblock_info, 0, sizeof(mblock_info_t));

	/*
	 * non-4GB mode case
	 */
	/* we do some DRAM size fixup here base on orig_dram_info */
	for (i = 0; i < orig_dram_info->rank_num; i++) {
		size += orig_dram_info->rank_info[i].size;
		mblock_info->mblock[i].start =
		    orig_dram_info->rank_info[i].start;
		mblock_info->mblock[i].rank = i;    /* setup rank */
		if (size <= max_dram_size) {
			mblock_info->mblock[i].size =
			    orig_dram_info->rank_info[i].size;
		} else {
			/* max dram size reached */
			size -= orig_dram_info->rank_info[i].size;
			mblock_info->mblock[i].size =
			    max_dram_size - size;
			/* get lca_reserved_mem info */
			lca_reserved_mem->start = mblock_info->mblock[i].start
			                          + mblock_info->mblock[i].size;
			if (mblock_info->mblock[i].size) {
				mblock_info->mblock_num++;
			}
			break;
		}

		if (mblock_info->mblock[i].size) {
			mblock_info->mblock_num++;
		}
	}

	pal_log_info("total_dram_size: 0x%llx, max_dram_size: 0x%llx\n",
	       total_dram_size, max_dram_size);
	if (total_dram_size > max_dram_size) {
		/* add left unused memory to lca_reserved memory */
		lca_reserved_mem->size = total_dram_size - max_dram_size;
		pal_log_info("lca_reserved_mem start: 0x%llx, size: 0x%llx\n",
		       lca_reserved_mem->start,
		       lca_reserved_mem->size);
	}

	/*
	 * TBD
	 * for 4GB mode, we fixup the start address of every mblock
	 */
}

/*
 * reserve a memory from mblock
 * @mblock_info: address of mblock_info
 * @reserved_size: size of memory
 * @align: alignment
 * @limit: address limit. Must higher than return address + reserved_size
 * @rank: preferable rank, the returned address is in rank or lower ranks
 * It returns as high rank and high address as possible. (consider rank first)
 */
u64 mblock_reserve(mblock_info_t *mblock_info, u64 reserved_size, u64 align, u64 limit,
                   enum reserve_rank rank)
{
	int i, max_rank, target = -1;
	u64 start, end, sz, max_addr = 0;
	u64 reserved_addr = 0;
	mblock_t mblock;

	if (mblock_info->mblock_num == 128) {
		/* the mblock[] is full */
		pal_log_err("mblock_reserve error: mblock[] is full\n");
		return 0;
	}

	if (!align)
		align = 0x1000;
	/* must be at least 4k aligned */
	if (align & (0x1000 - 1))
		align &= ~(0x1000 - 1);

	if (rank == RANK0) {
		/* reserve memory from rank 0 */
		max_rank = 0;
	} else {
		/* reserve memory from any possible rank */
		/* mblock_num >= nr_ranks is true */
		max_rank = mblock_info->mblock_num - 1;
	}

	for (i = 0; i < mblock_info->mblock_num; i++) {
		start = mblock_info->mblock[i].start;
		sz = mblock_info->mblock[i].size;
		end = limit < (start + sz)? limit: (start + sz);
		reserved_addr = (end - reserved_size);
		reserved_addr &= ~(align - 1);
		pal_log_info("mblock[%d].start: 0x%llx, sz: 0x%llx, limit: 0x%llx, "
		       "max_addr: 0x%llx, max_rank: %d, target: %d, "
		       "mblock[].rank: %d, reserved_addr: 0x%llx,"
		       "reserved_size: 0x%llx\n",
		       i, start, sz, limit, max_addr, max_rank,
		       target, mblock_info->mblock[i].rank,
		       reserved_addr, reserved_size);
		pal_log_info("mblock_reserve dbg[%d]: %d, %d, %d, %d, %d\n",
		       i, (reserved_addr + reserved_size <= start + sz),
		       (reserved_addr >= start),
		       (mblock_info->mblock[i].rank <= max_rank),
		       (start + sz > max_addr),
		       (reserved_addr + reserved_size <= limit));
		if ((reserved_addr + reserved_size <= start + sz) &&
		        (reserved_addr >= start) &&
		        (mblock_info->mblock[i].rank <= max_rank) &&
		        (start + sz > max_addr) &&
		        (reserved_addr + reserved_size <= limit)) {
			max_addr = start + sz;
			target = i;
		}
	}

	if (target < 0) {
		pal_log_err("mblock_reserve error\n");
		return 0;
	}

	start = mblock_info->mblock[target].start;
	sz = mblock_info->mblock[target].size;
	end = limit < (start + sz)? limit: (start + sz);
	reserved_addr = (end - reserved_size);
	reserved_addr &= ~(align - 1);

	/* split mblock if necessary */
	if (reserved_addr == start) {
		/*
		 * only needs to fixup target mblock
		 * [reserved_addr, reserved_size](reserved) +
		 * [reserved_addr + reserved_size, sz - reserved_size]
		 */
		mblock_info->mblock[target].start = reserved_addr + reserved_size;
		mblock_info->mblock[target].size -= reserved_size;
	} else if ((reserved_addr + reserved_size) == (start + sz)) {
		/*
		 * only needs to fixup target mblock
		 * [start, reserved_addr - start] +
		 * [reserved_addr, reserved_size](reserved)
		 */
		mblock_info->mblock[target].size = reserved_addr - start;
	} else {
		/*
		 * fixup target mblock and create a new mblock
		 * [start, reserved_addr - start] +
		 * [reserved_addr, reserved_size](reserved) +
		 * [reserved_addr + reserved_size, start + sz - reserved_addr - reserved_size]
		 */
		/* fixup original mblock */
		mblock_info->mblock[target].size = reserved_addr - start;

		/* new mblock */
		mblock.rank =  mblock_info->mblock[target].rank;
		mblock.start = reserved_addr + reserved_size;
		mblock.size = start + sz - (reserved_addr + reserved_size);

		/* insert the new node, keep the list sorted */
		memmove(&mblock_info->mblock[target + 2],
		        &mblock_info->mblock[target + 1],
		        sizeof(mblock_t) *
		        (mblock_info->mblock_num - target - 1));
		mblock_info->mblock[target + 1] = mblock;
		mblock_info->mblock_num += 1;
		pal_log_info( "mblock[%d]: %llx, %llx from mblock\n"
		        "mblock[%d]: %llx, %llx from mblock\n",
		        target,
		        mblock_info->mblock[target].start,
		        mblock_info->mblock[target].size,
		        target + 1,
		        mblock_info->mblock[target + 1].start,
		        mblock_info->mblock[target + 1].size);
	}

	pal_log_info("mblock_reserve: %llx - %llx from mblock %d\n",
	       reserved_addr, reserved_addr + reserved_size,
	       target);

	/* pal_log_info debug info */
	for (i = 0; i < mblock_info->mblock_num; i++) {
		start = mblock_info->mblock[i].start;
		sz = mblock_info->mblock[i].size;
		pal_log_info("mblock-debug[%d].start: 0x%llx, sz: 0x%llx\n",
		       i, start, sz);
	}

	return reserved_addr;
}
#if 0
/*
 * mblock_resize - resize mblock started at addr from oldsize to newsize,
 * current implementation only consider oldsize >= newsize.
 *
 * @mblock_info: mblock information
 * @orig_dram_info: original dram information
 * @addr: start address of a mblock
 * @oldsize: origianl size of the mblock
 * @newsize: new size of the given block
 * return 0 on success, otherwise 1
 */
int mblock_resize(mblock_info_t *mblock_info, dram_info_t *orig_dram_info,
                  u64 addr, u64 oldsize, u64 newsize)
{
	int err = 1;
	unsigned int i;
	u64 start, sz;
	mblock_t mblock;

	/* check size, oldsize must larger than newsize */
	if (oldsize <= newsize) {
		pal_log_info("mblock_resize error: mblock %llx oldsize(%llx) <= newsize(%llx)",
		       addr, oldsize, newsize);
		goto error;
	}

	/* check alignment, at least 4k aligned */
	if ((oldsize & (0x1000 - 1)) || (newsize & (0x1000 - 1))) {
		pal_log_info("mblock_resize alignment error: oldsize(%llx) or newsize(%llx)\n",
		       oldsize, newsize);
		goto error;
	}

	/* check mblock */
	for (i = 0; i < mblock_info->mblock_num; i++) {
		start = mblock_info->mblock[i].start;
		sz = mblock_info->mblock[i].size;
		/* invalid mblock */
		if ((addr >= start) && ((addr + oldsize) <= (start + sz))) {
			pal_log_info("mblock_resize error: mblock %llx, size: %llx is free\n",
			       addr, oldsize);
			goto error;
		}
	}

	/*
	 * ok, the mblock is valid and oldsize > newsize, let's
	 * shrink this mblock
	 */
	/* setup a new mblock */
	mblock.start = addr + newsize;
	mblock.size = oldsize - newsize;
	pal_log_info("mblock_resize putback mblock %llx size: %llx\n",
	       mblock.start, mblock.size);
	/* setup rank */
	for (i = 0; i < orig_dram_info->rank_num; i++) {
		start = orig_dram_info->rank_info[i].start;
		sz = orig_dram_info->rank_info[i].size;
		if ((mblock.start >= start) && ((mblock.start + mblock.size) <= (start + sz))) {
			mblock.rank = i;
			break;
		}
	}
	if (i >= orig_dram_info->rank_num) {
		pal_log_info("mblock_resize error: mblock not in orig_dram_info: %llx, size(%llx)\n",
		       mblock.start, mblock.size);
		goto error;
	}

	/* put the mblock back to mblock_info */
	for (i = 0; i < mblock_info->mblock_num; i++) {
		start = mblock_info->mblock[i].start;
		sz = mblock_info->mblock[i].size;
		if (mblock.rank == mblock_info->mblock[i].rank) {
			if (mblock.start == start + sz) {
				/*
				 * the new mblock can be merged to this mblock
				 * [start, start + sz] +
				 * [mblock.start, mblock.start + mblock.size](new)
				 */
				mblock_info->mblock[i].size += mblock.size;
				/* destroy block */
				mblock.size = 0;
			} else if (start == mblock.start + mblock.size) {
				/*
				 * the new mblock can be merged to this mblock
				 * [mblock.start, mblock.start + * mblock.size](new) +
				 * [start, start + sz]
				 */
				mblock_info->mblock[i].start = mblock.start;
				mblock_info->mblock[i].size += mblock.size;
				/* destroy block */
				mblock.size = 0;
			}
		}
	}

	/*
	 * mblock cannot be merge info mblock_info, insert it into mblock_info
	 */
	if (mblock.size) {
		for (i = 0; i < mblock_info->mblock_num; i++) {
			if (mblock.start < mblock_info->mblock[i].start)
				break;
		}
		memmove(&mblock_info->mblock[i + 1],
		        &mblock_info->mblock[i],
		        sizeof(mblock_t) *
		        (mblock_info->mblock_num - i));
		mblock_info->mblock[i] = mblock;
		mblock_info->mblock_num += 1;
	}

	/* pal_log_info debug info */
	for (i = 0; i < mblock_info->mblock_num; i++) {
		start = mblock_info->mblock[i].start;
		sz = mblock_info->mblock[i].size;
		pal_log_info("mblock-resize-debug[%d].start: 0x%llx, sz: 0x%llx\n",
		       i, start, sz);
	}

	return 0;
error:
	return err;
}

#endif

void mblock_set_mem_tag(unsigned int ** ptr)
{
	int i;
    boot_tag *tags = (boot_tag *)*ptr;
	memset(tags,0,sizeof(boot_tag));
    tags->hdr.size = boot_tag_size(boot_tag_mem);
    tags->hdr.tag = BOOT_TAG_MEM;
    tags->u.mem.dram_rank_num = bootarg.orig_dram_info.rank_num;
    for (i = 0; i < 4 ; i++) {
	bootarg.dram_rank_size[i] = bootarg.mblock_info.mblock[i].size;
    }
    for(i=0;i<4;i++){
	tags->u.mem.dram_rank_size[i] = bootarg.dram_rank_size[i];
    }
    tags->u.mem.mblock_info.mblock_num = bootarg.mblock_info.mblock_num;
    for(i=0;i<tags->u.mem.mblock_info.mblock_num;i++) {
	tags->u.mem.mblock_info.mblock[i].start = bootarg.mblock_info.mblock[i].start;
	tags->u.mem.mblock_info.mblock[i].size = bootarg.mblock_info.mblock[i].size;
	tags->u.mem.mblock_info.mblock[i].rank = bootarg.mblock_info.mblock[i].rank;
    }
#if defined(CFG_MBLOCK_LIB) && (MBLOCK_EXPAND(CFG_MBLOCK_LIB) == MBLOCK_EXPAND(2))
	tags->u.mem.mblock_info.mblock_magic = MBLOCK_MAGIC;
	tags->u.mem.mblock_info.mblock_version = MBLOCK_VERSION;
    tags->u.mem.mblock_info.reserved_num = bootarg.mblock_info.reserved_num;
    for(i=0;i<tags->u.mem.mblock_info.reserved_num;i++) {
	tags->u.mem.mblock_info.reserved[i].start = bootarg.mblock_info.reserved[i].start;
	tags->u.mem.mblock_info.reserved[i].size = bootarg.mblock_info.reserved[i].size;
	tags->u.mem.mblock_info.reserved[i].mapping= bootarg.mblock_info.reserved[i].mapping;
	memcpy(&tags->u.mem.mblock_info.reserved[i].name, &bootarg.mblock_info.reserved[i].name, MBLOCK_RESERVED_NAME_SIZE);
    }
#endif
    tags->u.mem.orig_dram_info.rank_num = bootarg.orig_dram_info.rank_num;
    for(i=0;i<4;i++) {
	tags->u.mem.orig_dram_info.rank_info[i].start = bootarg.orig_dram_info.rank_info[i].start; // 64
	tags->u.mem.orig_dram_info.rank_info[i].size = bootarg.orig_dram_info.rank_info[i].size; // 64
    }

    tags->u.mem.lca_reserved_mem.start= bootarg.lca_reserved_mem.start;
    tags->u.mem.lca_reserved_mem.size = bootarg.lca_reserved_mem.size;
    tags->u.mem.tee_reserved_mem.start= bootarg.tee_reserved_mem.start;
    tags->u.mem.tee_reserved_mem.size = bootarg.tee_reserved_mem.size;
    *ptr += tags->hdr.size;
}

void mblock_dump_mem_tag(unsigned int *ptr)
{
	boot_tag *tags = (boot_tag *)ptr;
	int i;
	pal_log_info("MEM_NUM: %d\n", tags->u.mem.dram_rank_num);
	for (i = 0; i < tags->u.mem.dram_rank_num; i++)
		pal_log_info("MEM_SIZE: 0x%x\n", tags->u.mem.dram_rank_size[i]);

	pal_log_info("mblock num: 0x%x\n", tags->u.mem.mblock_info.mblock_num);
	for(i=0;i<tags->u.mem.mblock_info.mblock_num;i++) {
		pal_log_info("mblock start: 0x%llx\n", tags->u.mem.mblock_info.mblock[i].start);
		pal_log_info("mblock size: 0x%llx\n", tags->u.mem.mblock_info.mblock[i].size);
		pal_log_info("mblock rank: 0x%x\n", tags->u.mem.mblock_info.mblock[i].rank);
	}
#if defined(CFG_MBLOCK_LIB) && (MBLOCK_EXPAND(CFG_MBLOCK_LIB) == MBLOCK_EXPAND(2))
	pal_log_info("magic=0x%lx version=0x%lx reserve num: 0x%x\n", \
			tags->u.mem.mblock_info.mblock_magic, \
			tags->u.mem.mblock_info.mblock_version,\
			tags->u.mem.mblock_info.reserved_num);\
	for(i=0;i<tags->u.mem.mblock_info.reserved_num;i++) {
		pal_log_info("mblock start: 0x%llx\n", tags->u.mem.mblock_info.reserved[i].start);
		pal_log_info("mblock size: 0x%llx\n", tags->u.mem.mblock_info.reserved[i].size);
		pal_log_info("mblock mapping: 0x%x\n", tags->u.mem.mblock_info.reserved[i].mapping);
		pal_log_info("mblock name: %s\n", tags->u.mem.mblock_info.reserved[i].name);
	}
#endif
	pal_log_info("orig_dram num: 0x%x\n", tags->u.mem.orig_dram_info.rank_num);
	for(i=0;i<4;i++) {
		pal_log_info("orig_dram start: 0x%llx\n", tags->u.mem.orig_dram_info.rank_info[i].start);
		pal_log_info("orig_dram size: 0x%llx\n", tags->u.mem.orig_dram_info.rank_info[i].size);
	}

	pal_log_info("lca start: 0x%llx\n", tags->u.mem.lca_reserved_mem.start);
	pal_log_info("lca size: 0x%llx\n", tags->u.mem.lca_reserved_mem.size);
	pal_log_info("tee start: 0x%llx\n", tags->u.mem.tee_reserved_mem.start);
	pal_log_info("tee size: 0x%llx\n", tags->u.mem.tee_reserved_mem.size);
}

#if 0
/*
 * mblock_create - create mblock started at addr or merge with existing mblock
 *
 * @mblock_info: mblock information
 * @orig_dram_info: original dram information
 * @addr: start address of a mblock, must be 4k align
 * @size: size of the given block, must be 4K align
 * return 0 on success, otherwise 1
 */
int mblock_create(mblock_info_t *mblock_info, dram_info_t *orig_dram_info
                  , u64 addr, u64 size)
{
	int err = -1;
	unsigned i,valid;
	u64 start, sz;
	mblock_t mblock;
	mblock_t *mblock_candidate_left = NULL, *mblock_candidate_right = NULL;

	/* check size, addr valid and align with 4K*/
	if (!size || size&(0x1000 - 1) || addr&(0x1000 - 1) ) {
		pal_log_info("mblock_create size invalid size=%llx\n",size);
		goto error;
	}
	/* for lca check*/
	if (g_boot_arg->lca_reserved_mem.start && g_boot_arg->lca_reserved_mem.size) {
		if ((addr >= g_boot_arg->lca_reserved_mem.start && addr < g_boot_arg->lca_reserved_mem.start \
		        + g_boot_arg->lca_reserved_mem.size)|| \
		        (addr + size > g_boot_arg->lca_reserved_mem.start \
		         && (addr + size) < g_boot_arg->lca_reserved_mem.start + g_boot_arg->lca_reserved_mem.size) ) {
			pal_log_info("mblock_create ERROR , overlap with LCA addr and size invalid addr = %llx size=%llx\n", addr, size);
			goto error;
		}
	}

	/* for tee check*/
	if (g_boot_arg->tee_reserved_mem.start && g_boot_arg->tee_reserved_mem.size) {
		if ((addr >= g_boot_arg->tee_reserved_mem.start && addr < g_boot_arg->tee_reserved_mem.start \
		        + g_boot_arg->tee_reserved_mem.size)|| \
		        (addr + size > g_boot_arg->tee_reserved_mem.start \
		         && (addr + size) < g_boot_arg->tee_reserved_mem.start + g_boot_arg->tee_reserved_mem.size) ) {
			pal_log_info("mblock_create ERROR , overlap with tee addr and size invalid addr = %llx size=%llx\n", addr, size);
			goto error;
		}
	}

	/*it's not allow to create mblock which is cross rank
	 * and mblock should not exceed rank size */
	for (i = 0, valid = 0; i < orig_dram_info->rank_num; i++) {
		start = orig_dram_info->rank_info[i].start;
		sz = orig_dram_info->rank_info[i].size;
		if (addr >= start && addr < start + sz && addr + size <= start + sz) {
			valid =1;
			break;
		}
	}
	if (!valid) {
		pal_log_info("mblock_create addr and size invalid addr=%llx size=%llx\n",
		        addr,size);
		goto error;
	}

	/* check every mblock the addr and size should not be within any existing mblock */
	for (i = 0; i < mblock_info->mblock_num; i++) {
		start = mblock_info->mblock[i].start;
		sz = mblock_info->mblock[i].size;
		/*addr should start from reserved memory space and addr + size should not overlap with mblock
		 * when addr is smaller than start*/
		if (((addr >= start) && (addr < start + sz)) || (addr < start && addr + size > start)) {
			pal_log_info("mblock_create error: addr %llx overlap with mblock %llx, size: %llx \n",
			        addr, start, sz);
			goto error;
		}
	}

	/*
	 * ok, the mblock is valid let's create the mblock
	 * and try to merge it with the same bank and choose the bigger size one
	 */
	/* setup a new mblock */
	mblock.start = addr;
	mblock.size = size;
	pal_log_info("mblock_create mblock start %llx size: %llx\n",
	        mblock.start, mblock.size);
	/* setup rank */
	for (i = 0; i < orig_dram_info->rank_num; i++) {
		start = orig_dram_info->rank_info[i].start;
		sz = orig_dram_info->rank_info[i].size;
		if ((mblock.start >= start) && ((mblock.start + mblock.size) <= (start + sz))) {
			mblock.rank = i;
			break;
		}
	}
	if (i >= orig_dram_info->rank_num) {
		pal_log_info("mblock_create error: mblock not in orig_dram_info: %llx, size(%llx)\n",
		        mblock.start, mblock.size);
		goto error;
	}

	/* put the mblock back to mblock_info */
	for (i = 0; i < mblock_info->mblock_num; i++) {
		start = mblock_info->mblock[i].start;
		sz = mblock_info->mblock[i].size;
		if (mblock.rank == mblock_info->mblock[i].rank) {
			if (mblock.start + mblock.size == start) {
				/*
				 * the new mblock could be merged to this mblock
				 */
				mblock_candidate_right = &mblock_info->mblock[i];
			} else if (start + sz == mblock.start) {
				/*
				 * the new mblock can be merged to this mblock
				 */
				mblock_candidate_left =  &mblock_info->mblock[i];
			}
		}
	}
	/*we can merge either left or right , choose the bigger one */
	if (mblock_candidate_right && mblock_candidate_left) {
		if (mblock_candidate_right->size >= mblock_candidate_left->size) {
			pal_log_info("mblock_candidate_right->size = %llx \
				mblock_candidate_left->size = %llx \n",mblock_candidate_right->size, mblock_candidate_left->size);
			mblock_candidate_right->start = mblock.start;
			mblock_candidate_right->size += mblock.size;
		} else { /*left bigger*/
			pal_log_info("mblock_candidate_right->size = %llx \
				mblock_candidate_left->size = %llx \n",mblock_candidate_right->size, mblock_candidate_left->size);
			mblock_candidate_left->size += mblock.size;
		}
		/* destroy block */
		mblock.size = 0;
	} else {
		if (mblock_candidate_right) {
			mblock_candidate_right->start = mblock.start;
			mblock_candidate_right->size += mblock.size;
			/* destroy block */
			mblock.size = 0;
		}

		if (mblock_candidate_left) {
			mblock_candidate_left->size += mblock.size;
			/* destroy block */
			mblock.size = 0;
		}
	}

	/*
	 * mblock cannot be merge into mblock_info, insert it into mblock_info
	 */
	if (mblock.size) {
		for (i = 0; i < mblock_info->mblock_num; i++) {
			if (mblock.start < mblock_info->mblock[i].start)
				break;
		}
		/* insert the new node, keep the list sorted */
		if (i != mblock_info->mblock_num) {
			memmove(&mblock_info->mblock[i + 1],
			        &mblock_info->mblock[i],
			        sizeof(mblock_t) *
			        (mblock_info->mblock_num - i));
		}
		mblock_info->mblock[i] = mblock;
		mblock_info->mblock_num += 1;
		pal_log_info( "create mblock[%d]: %llx, %llx \n",
		        i,
		        mblock_info->mblock[i].start,
		        mblock_info->mblock[i].size);
	}

	/* pal_log_info debug info */
	for (i = 0; i < mblock_info->mblock_num; i++) {
		start = mblock_info->mblock[i].start;
		sz = mblock_info->mblock[i].size;
		pal_log_info("mblock-create-debug[%d].start: 0x%llx, sz: 0x%llx\n",
		        i, start, sz);
	}

	return 0;
error:
	return err;
}
#endif

