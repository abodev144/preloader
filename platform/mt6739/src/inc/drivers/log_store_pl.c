/* This software/firmware and related documentation ("MediaTek Software") are
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
#include <platform.h>
#include "partition.h"
#include "log_store_pl.h"
#include "dram_buffer.h"

#define MOD "PL_LOG_STORE"
#define AEE_IPANIC_PLABLE "expdb"

//#define   PRINT_EARLY_KERNEL_LOG
#define EMMC_LOG_SIZE (LOG_STORE_SIZE * 2)

#define DEBUG_LOG

// !!!!!!! Because log store be called by print, so these function don't use print log to debug.
#ifdef DEBUG_LOG
#define LOG_DEBUG(fmt, ...) \
    log_store_enable = false; \
    print(fmt, ##__VA_ARGS__); \
    log_store_enable = true
#else
#define LOG_DEBUG(fmt, ...)
#endif

#define LOG_MEM_ALIGNMENT (0x1000)  //PAGE Alignment(4K)
#define EMMC_LOG_BUF_SIZE 0x200000

typedef enum {
	LOG_WRITE = 0x1,        // Log is write to buff
	LOG_READ_KERNEL = 0x2,  // Log have readed by kernel
	LOG_WRITE_EMMC = 0x4,   // log need save to emmc
	LOG_EMPTY = 0x8,        // log is empty
	LOG_FULL = 0x10,        // log is full
	LOG_PL_FINISH = 0X20,   // pl boot up finish
	LOG_LK_FINISH = 0X40,   // lk boot up finish
	LOG_DEFAULT = LOG_WRITE_EMMC|LOG_EMPTY,
} BLOG_FLAG;


//printk log store buff, 1 DRAM, 0 SRAM. only printk user DRAM,
// we can user printk SRAM buff to store log.
extern int  g_log_drambuf;
#define C_LOG_SRAM_BUF_SIZE (20480)
extern char log_sram_buf[C_LOG_SRAM_BUF_SIZE];
#define bootarg g_dram_buf->bootarg


static int log_store_status = BUFF_NOT_READY;
static struct sram_log_header *sram_header = (struct sram_log_header*)SRAM_LOG_ADDR;
static struct pl_lk_log *dram_curlog_header = &(((struct sram_log_header*)SRAM_LOG_ADDR)->dram_curlog_header);
static struct mirror_log_header *dram_mlog_header = &(((struct sram_log_header*)SRAM_LOG_ADDR)->dram_mlog_header);
static struct dram_buf_header *sram_dram_buff = &(((struct sram_log_header*)SRAM_LOG_ADDR)->dram_buf[LOG_PL_LK]);
static char *pbuff = NULL;
static int log_store_sram = 1;
static int sram_store_count = 0;
static bool log_store_enable = true;

void format_log_buff(void)
{
	memset(dram_curlog_header, 0, sizeof(struct pl_lk_log));
	dram_curlog_header->sig = LOG_STORE_SIG;
	dram_curlog_header->buff_size = LOG_STORE_SIZE;
	dram_curlog_header->off_pl = sizeof(struct pl_lk_log);
	dram_curlog_header->off_lk = sizeof(struct pl_lk_log);
	dram_curlog_header->pl_flag = LOG_DEFAULT;
	dram_curlog_header->lk_flag = LOG_DEFAULT;

	if (!mirrorbuf_valid()) {
		LOG_DEBUG("%s:mirror sig:0x%x, format mirror log buffer!\n", MOD, dram_mlog_header->sig);
		memset(dram_mlog_header, 0x0, LOG_STORE_SIZE_MIRROR);
		dram_mlog_header->sig = LOG_STORE_SIG;
		dram_mlog_header->count = 0;
		dram_mlog_header->mlog[0].start_addr = sram_dram_buff->mirror_header_addr;
		dram_mlog_header->mlog[0].size = 0;
		dram_mlog_header->mlog[1].start_addr = sram_dram_buff->mirror_header_addr + (LOG_STORE_SIZE_MIRROR >> 1);
		dram_mlog_header->mlog[1].size = 0;
	}

	return;
}

int logbuf_valid(void)
{
	if ((dram_curlog_header != NULL) && (dram_curlog_header->sig == LOG_STORE_SIG) &&
		(dram_curlog_header->buff_size == LOG_STORE_SIZE) && (dram_curlog_header->off_pl == sizeof(struct pl_lk_log))
		&& ((dram_curlog_header->sz_lk + dram_curlog_header->sz_pl + dram_curlog_header->off_pl) <= LOG_STORE_SIZE)
		&& (dram_curlog_header->pl_flag <= 0x80 - 1) && (dram_curlog_header->lk_flag <= 0x80 - 1))
		return 1;
	return 0;
}

int mirrorbuf_valid(void)
{
	struct sram_log_header *sram_header = (struct sram_log_header*)SRAM_LOG_ADDR;

	if (dram_mlog_header->sig != LOG_STORE_SIG) {
		// LOG_DEBUG("%s:sram_dram_buff->sig 0x%x!\n",MOD,sram_dram_buff->sig);
		return 0;
	}
	if (dram_mlog_header->mlog[0].start_addr != sram_dram_buff->mirror_header_addr) {
		// LOG_DEBUG("%s:dram_mlog_header->mlog[0].start_addr 0x%x!\n",MOD,dram_mlog_header->mlog[0].start_addr);
		return 0;
	}
	if (dram_mlog_header->mlog[0].size > (LOG_STORE_SIZE_MIRROR >> 1)) {
		// LOG_DEBUG("%s:dram_mlog_header->mlog[0].size 0x%x!\n",MOD,dram_mlog_header->mlog[0].size);
		return 0;
	}
	if (dram_mlog_header->mlog[1].start_addr != sram_dram_buff->mirror_header_addr+ (LOG_STORE_SIZE_MIRROR >> 1)) {
		// LOG_DEBUG("%s:dram_mlog_header->mlog[1].start_addr 0x%x!\n",MOD,dram_mlog_header->mlog[1].start_addr);
		return 0;
	}
	if (dram_mlog_header->mlog[1].size > (LOG_STORE_SIZE_MIRROR >> 1)) {
		// LOG_DEBUG("%s:dram_mlog_header->mlog[1].size 0x%x!\n",MOD,dram_mlog_header->mlog[1].size);
		return 0;
	}
	return 1;
}

void log_to_mirror_buffer(struct sram_log_header *sram_buff)
{
	if (sram_buff == NULL) {
		LOG_DEBUG("%s:sram_buff is null!\n",MOD);
		return;
	}
	if (sram_dram_buff == NULL) {
		LOG_DEBUG("%s:sram_dram_buff is NULL!\n",MOD);
		return;
	}

	if (sram_dram_buff->sig != DRAM_HEADER_SIG) {
		LOG_DEBUG("%s:sram_dram_buff->sig 0x%x!\n",MOD,sram_dram_buff->sig);
		return;
	}

	if ((u32 *)sram_dram_buff->buf_addr == NULL) {
		LOG_DEBUG("%s:sram_dram_buff->buf_addr is NULL!\n",MOD);
		return;
	}

	if (sram_dram_buff->buf_size != LOG_STORE_SIZE) {
		LOG_DEBUG("%s:error:sram_dram_buff->buf_size 0x%x not 0x%x!\n",MOD,sram_dram_buff->buf_size, LOG_STORE_SIZE);
		return;
	}

	pbuff = (char *)sram_dram_buff->buf_addr;
	dram_curlog_header = &(sram_header->dram_curlog_header);

	if (!logbuf_valid()) {
		LOG_DEBUG("%s:mirror:logbuf not ready!\n", MOD);
		return;
	}

	if (!mirrorbuf_valid()) {
		LOG_DEBUG("%s:mirror:mirrorbuf not ready!\n", MOD);
		return;
	}

	struct mirror_log_header *m_header = &(sram_header->dram_mlog_header);
	//struct mirror_log *m_log = (m_header->count == 0) ? (&m_header->mlog[0]) : (&m_header->mlog[1]);
	struct mirror_log *m_log = (m_header->count > 0) ? (&(m_header->mlog[1])) : (&(m_header->mlog[0]));

	//clear and move the mirror buffer
	if ((m_header->count > 0) && (m_header->mlog[1].size > 0)) {//both buffer was filled with logs
		LOG_DEBUG("%s:move mirror buffer. dst: mlog[0].start_addr=0x%x,size=0x%x; src: mlog[1].start_addr=0x%x, size=0x%x\n",
				MOD, m_header->mlog[0].start_addr, m_header->mlog[0].size, m_header->mlog[1].start_addr, m_header->mlog[1].size);
		memset((void *)m_header->mlog[0].start_addr, 0x0, m_header->mlog[0].size);
		memcpy((void *)m_header->mlog[0].start_addr, (void *)m_header->mlog[1].start_addr, m_header->mlog[1].size);
		m_header->mlog[0].size = m_header->mlog[1].size;
		memset((void *)m_header->mlog[1].start_addr, 0x0, m_header->mlog[1].size);
	}

	u32 src_addr = (u32)(pbuff + dram_curlog_header->off_pl); // (u32)(pbuff + dram_curlog_header->off_pl);
	u32 dst_addr = m_log->start_addr;
	u32 last_pl_lk_log_size = dram_curlog_header->sz_pl + dram_curlog_header->sz_lk; // dram_curlog_header->sz_pl + dram_curlog_header->sz_lk;
	//copy log to mirror buf
	if (last_pl_lk_log_size) {
		if (!dst_addr)
			return;

		if (last_pl_lk_log_size > (LOG_STORE_SIZE_MIRROR >> 1)) {
			last_pl_lk_log_size = (LOG_STORE_SIZE_MIRROR >> 1);
			LOG_DEBUG("%s:last log size overflow!", MOD);
		}

		memcpy((void *)dst_addr, (void *)src_addr, last_pl_lk_log_size);
		m_log->size = last_pl_lk_log_size;
		m_header->count++;
		LOG_DEBUG("%s:copy to mirror: dram_curlog_header:0x%x, dst addr:0x%x, src addr:0x%x, len:0x%x, copy count:%d\n", 
				MOD, dram_curlog_header, dst_addr, src_addr, last_pl_lk_log_size, m_header->count);
	}
}

// store log buff to emmc
void log_to_emmc(struct sram_log_header *sram_buff)
{
	int i;
	char *value;
	part_t *part_ptr;
	blkdev_t *bootdev;
	u32 log_emmc_addr, part_start_addr, part_end_addr, part_size;
	u32 klog_buf_addr = 0, klog_buf_size = 0;
	u32 atf_log_buf_addr = 0, atf_log_buf_size = 0;
	int store_size = 0, emmc_remain_buf_size = 0;
	int ret = 0;

	LOG_DEBUG("%s:log_to_emmc function flag 0x%x!\n",MOD, sram_dram_buff->flag);

	if (sram_buff == NULL) {
		LOG_DEBUG("%s:sram_buff is null!\n",MOD);
		return;
	}
	if (sram_dram_buff == NULL) {
		LOG_DEBUG("%s:sram_dram_buff is NULL!\n",MOD);
		return;
	}

	if (sram_dram_buff->sig != DRAM_HEADER_SIG) {
		LOG_DEBUG("%s:sram_dram_buff->sig 0x%x!\n",MOD,sram_dram_buff->sig);
		return;
	}

	if ((u32 *)sram_dram_buff->buf_addr == NULL) {
		LOG_DEBUG("%s:sram_dram_buff->buf_addr is NULL!\n",MOD);
		return;
	}

	if (sram_dram_buff->buf_size != LOG_STORE_SIZE) {
		LOG_DEBUG("%s:error:sram_dram_buff->buf_size 0x%x not 0x%x!\n",MOD,sram_dram_buff->buf_size, LOG_STORE_SIZE);
		return;
	}

	if ((sram_dram_buff->flag & NEED_SAVE_TO_EMMC) != NEED_SAVE_TO_EMMC) {
		LOG_DEBUG("%s:don't need to store to emmc, flag 0x%x!\n",MOD,sram_dram_buff->flag);
		return;
	}
	sram_buff->reboot_count++;
	pbuff = (char *)sram_dram_buff->buf_addr;
	dram_curlog_header = &(sram_header->dram_curlog_header);

	if (!logbuf_valid()) {
		LOG_DEBUG("%s:logbuf not valid!\n", MOD);
		return;
	}

	struct mirror_log_header *m_header = (struct mirror_log_header *)(sram_dram_buff->mirror_header_addr);

	//save the top 3 times, after that, 10 times scale
	if (sram_buff->reboot_count >= sram_buff->save_to_emmc) {
		log_store_enable = false;
		bootdev = blkdev_get(CFG_BOOT_DEV);
		log_store_enable = true;
		if (NULL == bootdev) {
			LOG_DEBUG("%s can't find boot device(%d)\n", MOD, CFG_BOOT_DEV);
			return ;
		}
//		for(i = 0; i < MAX_DRAM_COUNT; i++)
		{
			log_store_enable = false;
			part_ptr = part_get(AEE_IPANIC_PLABLE);
			log_store_enable = true;
			if (part_ptr == NULL) {
				LOG_DEBUG("%s:log_to_emmc get partition error!\n",MOD);
			} else {
				char prepostfix[bootdev->blksz];
				#if ((CFG_BOOT_DEV == BOOTDEV_SDMMC) || (CFG_BOOT_DEV == BOOTDEV_UFS))
				part_start_addr = part_ptr->start_sect * bootdev->blksz;
				part_end_addr = (part_ptr->start_sect + part_ptr->nr_sects) * bootdev->blksz;
				part_size = part_ptr->nr_sects * bootdev->blksz;
				LOG_DEBUG("%s:%s partition start addr 0x%x, end addr 0x%x, partition size 0x%x, nr_sects 0x%x, blksz 0x%x!\n",
				          MOD, AEE_IPANIC_PLABLE, part_start_addr, part_end_addr, part_size, part_ptr->nr_sects, bootdev->blksz);
				#else
				part_start_addr = mt_part_get_start_addr(part_ptr) * bootdev->blksz;
				part_end_addr = (mt_part_get_start_addr(part_ptr)+ mt_part_get_size(part_ptr)) * bootdev->blksz;
				part_size = mt_part_get_size(part_ptr) * bootdev->blksz;
				LOG_DEBUG("%s:%s partition start addr 0x%x, end addr 0x%x, partition size 0x%x, nr_sects 0x%x, blksz 0x%x!\n",
				          MOD, AEE_IPANIC_PLABLE, part_start_addr, part_end_addr, part_size, mt_part_get_size(part_ptr), bootdev->blksz);
				#endif
				LOG_DEBUG("%s:last pl log size 0x%x, lk log size 0x%x!\n", MOD, dram_curlog_header->sz_pl, dram_curlog_header->sz_lk);

				//write unit is bootdev->blksz;
				/*The last 2MB of expdb*/
				log_emmc_addr = part_end_addr - EMMC_LOG_BUF_SIZE;
				/* remain 2 block size to save end flag*/
				emmc_remain_buf_size = EMMC_LOG_BUF_SIZE - 2 * bootdev->blksz;

				/* write prefix*/
				memset(prepostfix, 'M', sizeof(prepostfix) - 1);
				prepostfix[0] = prepostfix[sizeof(prepostfix) - 1] = '\n';
				store_size = sizeof(prepostfix);
				if (log_emmc_addr > part_start_addr && store_size > 0) {
					log_store_enable = false;
					ret = blkdev_write(bootdev, log_emmc_addr, store_size, prepostfix, part_ptr->part_id);
					log_store_enable = true;
					if (ret) {
						LOG_DEBUG("%s:write prefix error, ret value = 0x%x\n", MOD, ret);
						return;
					}
				} else {
					LOG_DEBUG("%s:write prefix log to emmc addr 0x%x, store_size 0x%x, beyond expdb range!\n", MOD, log_emmc_addr, store_size);
					return;
				}

				/* write preloader/lk log */
				log_emmc_addr += store_size;
				emmc_remain_buf_size -= store_size;

				if (emmc_remain_buf_size < 0)
					return;

				if ((LOG_STORE_SIZE_MIRROR) % bootdev->blksz)
					store_size = (LOG_STORE_SIZE_MIRROR / bootdev->blksz + 1) * bootdev->blksz;
				else
					store_size = LOG_STORE_SIZE_MIRROR;

				if (store_size > emmc_remain_buf_size) {
					if (emmc_remain_buf_size % bootdev->blksz)
						store_size = (emmc_remain_buf_size / bootdev->blksz + 1) * bootdev->blksz;
					else
						store_size = emmc_remain_buf_size;
				}

				/*check the write address boundary*/
				if (log_emmc_addr > part_start_addr && store_size > 0) {
					LOG_DEBUG("%s:pl/lk log_emmc_addr:0x%x,store_size:0x%x, m_header:0x%x\n",
							MOD,log_emmc_addr,store_size, (unsigned int)m_header);
					log_store_enable = false;
					ret = blkdev_write(bootdev, log_emmc_addr, store_size, (u8 *)m_header, part_ptr->part_id);
					log_store_enable = true;

					if (ret) {
						LOG_DEBUG("%s:blkdev_write pl/lk log error, ret value = 0x%x\n", MOD, ret);
						return;
					}
				} else {
					LOG_DEBUG("%s:write preloader/lk log to emmc addr 0x%x, store_size 0x%x, beyond expdb range!\n", MOD, log_emmc_addr, store_size);
					return;
				}

				/* write ATF log */
				if (sram_dram_buff->atf_log_addr != 0 && sram_dram_buff->atf_log_len != 0) {
					log_emmc_addr += store_size;
					emmc_remain_buf_size -= store_size;

					if (emmc_remain_buf_size < 0)
						return;

					atf_log_buf_addr = sram_dram_buff->atf_log_addr;
					atf_log_buf_size = sram_dram_buff->atf_log_len;

					if (atf_log_buf_size % bootdev->blksz)
						store_size = (LOG_STORE_SIZE_MIRROR / bootdev->blksz + 1) * bootdev->blksz;
					else
						store_size = LOG_STORE_SIZE_MIRROR;

					if (store_size > emmc_remain_buf_size) {
						if (emmc_remain_buf_size % bootdev->blksz)
							store_size = (emmc_remain_buf_size / bootdev->blksz + 1) * bootdev->blksz;
						else
							store_size = emmc_remain_buf_size;
					}

					/*check the write address boundary*/
					if (log_emmc_addr > part_start_addr && store_size > 0) {
						LOG_DEBUG("%s:ATF log_emmc_addr:0x%x,store_size:0x%x, atf log buf addr:0x%x, atf log buf size:0x%x\n",
								MOD,log_emmc_addr,store_size, (unsigned int)atf_log_buf_addr, (unsigned int)atf_log_buf_size);
						log_store_enable = false;
						ret = blkdev_write(bootdev, log_emmc_addr, store_size, (u8 *)atf_log_buf_addr, part_ptr->part_id);
						log_store_enable = true;

						if (ret) {
							LOG_DEBUG("%s:blkdev_write ATF log error, ret value = 0x%x\n", MOD, ret);
							return;
						}
					} else {
						LOG_DEBUG("%s:write ATF log to emmc addr 0x%x, store_size 0x%x, beyond expdb range!\n", MOD, log_emmc_addr, store_size);
						return;
					}
				}

				/* write kernel log */
				if ((sram_dram_buff->flag & BUFF_EARLY_PRINTK) && sram_dram_buff->klog_addr != 0
				        && sram_dram_buff->klog_size != 0) {
					log_emmc_addr += store_size;
					emmc_remain_buf_size -= store_size;

					if (emmc_remain_buf_size < 0)
						return;

					klog_buf_addr = sram_dram_buff->klog_addr;
					klog_buf_size = sram_dram_buff->klog_size;

					store_size = klog_buf_size < emmc_remain_buf_size ? klog_buf_size : emmc_remain_buf_size;

					if (store_size % bootdev->blksz) {
						store_size = (store_size / bootdev->blksz + 1) * bootdev->blksz;
					}

					if (store_size > emmc_remain_buf_size) {
						if (emmc_remain_buf_size % bootdev->blksz)
							store_size = (emmc_remain_buf_size / bootdev->blksz + 1) * bootdev->blksz;
						else
							store_size = emmc_remain_buf_size;
					}

					/*check the write address boundary*/
					if (log_emmc_addr > part_start_addr && store_size > 0) {
						LOG_DEBUG("%s:kernel log_emmc_addr:0x%x,store_size:0x%x, klog buf addr:0x%x, klog buf size:0x%x\n",
								MOD,log_emmc_addr,store_size, (unsigned int)klog_buf_addr, (unsigned int)klog_buf_size);
						log_store_enable = false;
						ret = blkdev_write(bootdev, log_emmc_addr, store_size, (u8 *)klog_buf_addr, part_ptr->part_id);
						log_store_enable = true;
						if (ret) {
							LOG_DEBUG("%s:blkdev_write kernel log error, ret value = 0x%x\n", MOD, ret);
							return;
						}
					} else {
						LOG_DEBUG("%s:write kernel log to emmc addr 0x%x, store_size 0x%x, beyond expdb range!\n", MOD, log_emmc_addr, store_size);
						return;
					}
				}

				log_emmc_addr += store_size;
				memset(prepostfix, '=', sizeof(prepostfix) - 1);
				prepostfix[0] = prepostfix[sizeof(prepostfix) - 1] = '\n';
				store_size = sizeof(prepostfix);
				/*log end flag*/
				if (log_emmc_addr > part_start_addr && store_size > 0) {
					log_store_enable = false;
					ret = blkdev_write(bootdev, log_emmc_addr, store_size, prepostfix, part_ptr->part_id);
					log_store_enable = true;
					if (ret) {
						LOG_DEBUG("%s:write log end flag error, ret value = 0x%x\n", MOD, ret);
						return;
					}
				}

				memset(prepostfix, 'W', sizeof(prepostfix) - 1);
				prepostfix[0] = prepostfix[sizeof(prepostfix) - 1] = '\n';
				log_store_enable = false;
				ret = blkdev_write(bootdev, part_end_addr - sizeof(prepostfix), sizeof(prepostfix), prepostfix, part_ptr->part_id);
				log_store_enable = true;
				if (ret) {
					LOG_DEBUG("%s:write postfix error, ret value = 0x%x\n", MOD, ret);
					return;
				}
			}
		}
		if (sram_buff->reboot_count >= 3)
			sram_buff->save_to_emmc = 5 * sram_buff->reboot_count;
	} else {
		LOG_DEBUG("%s:reboot_count %d,save_to_emmc %d.\n", MOD, sram_buff->reboot_count, sram_buff->save_to_emmc);
	}
	return;
}

#ifdef PRINT_EARLY_KERNEL_LOG
void kernel_log_show(void)
{
	int i = 0;
	char value;
	if (sram_dram_buff == NULL)
		return;

	// print early printk message
	if ((sram_dram_buff->flag & BUFF_EARLY_PRINTK) && sram_dram_buff->klog_addr != 0
	        && sram_dram_buff->klog_size != 0) {
		log_store_enable = false;
		for (i=0; i < sram_dram_buff->klog_size; i++) {
			value = *((char *)sram_dram_buff->klog_addr+i);
			print("%c",value);
		}
		log_store_enable = true;
	}

}
#endif

void log_store_init(void)
{
	if (log_store_status != BUFF_NOT_READY) {
		LOG_DEBUG("%s:log_sotore_status is ready!\n",MOD);
		return;
	}
// SRAM buff header init
	sram_header = (struct sram_log_header*)SRAM_LOG_ADDR;
	LOG_DEBUG("%s:sram->sig value 0x%x!\n",MOD,sram_header->sig);
	if (sram_header->sig != SRAM_HEADER_SIG) {
		memset(sram_header,0, sizeof(struct sram_log_header));
		LOG_DEBUG("%s:sram header is not match, format all!\n",MOD);
		sram_header->sig = SRAM_HEADER_SIG;
		LOG_DEBUG("%s:set ram_header->sig = 0x%x\n", MOD, sram_header->sig);
	}

	//save log to mirror buffer
	log_to_mirror_buffer(sram_header);
	// Save log to emmc
	log_to_emmc(sram_header);

#ifdef PRINT_EARLY_KERNEL_LOG
	kernel_log_show();
#endif
	memset(sram_dram_buff, 0, sizeof(struct dram_buf_header));
	sram_dram_buff->sig = DRAM_HEADER_SIG;

	log_store_enable = false;
	pbuff = (char *)((u32)mblock_reserve_ext(&bootarg.mblock_info,
	                            (u64)(LOG_STORE_SIZE + LOG_STORE_SIZE_MIRROR), (u64)LOG_MEM_ALIGNMENT, 0x80000000, 1, "log_store"));
	log_store_enable = true;

	if (!pbuff) {
		LOG_DEBUG("%s:dram log allocation error!\n",MOD);
		sram_dram_buff->flag = BUFF_ALLOC_ERROR;
		log_store_status = BUFF_ALLOC_ERROR;
		return;
	}

	memset(pbuff, 0, LOG_STORE_SIZE);
	sram_dram_buff->buf_addr = (u32)pbuff;
	sram_dram_buff->buf_offsize = sizeof(struct pl_lk_log);
	sram_dram_buff->buf_size = LOG_STORE_SIZE;
	sram_dram_buff->flag = BUFF_VALID | CAN_FREE | NEED_SAVE_TO_EMMC | ARRAY_BUFF;
	sram_dram_buff->buf_point = 0;
	sram_dram_buff->mirror_header_addr = (u32)(pbuff + LOG_STORE_SIZE);

	// init DRAM buff
	format_log_buff();
	log_store_status = BUFF_READY;

	LOG_DEBUG("%s:sram_header 0x%x,sig 0x%x, sram_dram_buff 0x%x, buf_addr 0x%x; dram_mlog_header=0x%x, sig=0x%x, count=0x%x, mlog[0].start_addr=0x%x,size=0x%x, mlog[1].start_addr=0x%x,size=0x%x\n", MOD,\
		sram_header,sram_header->sig,sram_dram_buff,sram_dram_buff->buf_addr,\
		dram_mlog_header, dram_mlog_header->sig, dram_mlog_header->count, dram_mlog_header->mlog[0].start_addr,dram_mlog_header->mlog[0].size,\
		dram_mlog_header->mlog[1].start_addr, dram_mlog_header->mlog[1].size);
	return;
}

void store_switch_to_dram(void)
{
	int i=0;
	log_store_sram = 0;
	log_store_init();
	if (g_log_drambuf == 1) {
		for (i=0; i < sram_store_count; i++) {
			pl_log_store(log_sram_buf[i]);
		}
	}
}

void pl_log_store(char c)
{
	if (log_store_enable == false) {
		return;
	}

	if ((log_store_status == BUFF_ALLOC_ERROR) || (log_store_status == BUFF_FULL))
		return;

	if ((log_store_sram ==1) && (g_log_drambuf == 1)) {
		if (sram_store_count < C_LOG_SRAM_BUF_SIZE) {
			log_sram_buf[sram_store_count++] = c;
		}

		return;
	}

	if (log_store_status == BUFF_NOT_READY) {
		log_store_init();
		return;
	}

	if (logbuf_valid() == 0) {
		return;
	}

	if (log_store_status != BUFF_READY) {
		return;
	}

	if(!mirrorbuf_valid()) {
		return;
	}

	*(pbuff + dram_curlog_header->off_pl + dram_curlog_header->sz_pl) = c;
	dram_curlog_header->sz_pl++; // dram_curlog_header->sz_pl++;
	sram_dram_buff->buf_point = dram_curlog_header->sz_pl; // dram_curlog_header->sz_pl;
	if ((dram_curlog_header->off_pl + dram_curlog_header->sz_pl) >= LOG_STORE_SIZE) {
		log_store_status = BUFF_FULL;
		// LOG_DEBUG("%s: dram buff full", MOD);
	}

	return;
}
