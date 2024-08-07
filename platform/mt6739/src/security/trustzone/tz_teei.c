/*
 * Copyright (c) 2015-2016 MICROTRUST Incorporated
 * All rights reserved
 *
 * This file and software is confidential and proprietary to MICROTRUST Inc.
 * Unauthorized copying of this file and software is strictly prohibited.
 * You MUST NOT disclose this file and software unless you get a license
 * agreement from MICROTRUST Incorporated.
 */

#include <typedefs.h>
#include <tz_mem.h>
#include <tz_teei.h>
#include <uart.h>
#include <dram_buffer.h>
#include <utos_version.h>
#include <isee_msg_log.h>

/* #define TEEI_DEBUG 1 */
#if CFG_BOOT_ARGUMENT_BY_ATAG
extern unsigned int g_uart;
#elif CFG_BOOT_ARGUMENT && !CFG_BOOT_ARGUMENT_BY_ATAG
#define bootarg g_dram_buf->bootarg
#endif
void CONFIG_ISEE_DEVS(tee_dev_t_ptr devs, u32 type, u64 addr, u32 irq, u32 dapc_num, u32 param0)
{
	((tee_dev_t_ptr)devs)->dev_type = type;
	((tee_dev_t_ptr)devs)->base_addr = addr;
	((tee_dev_t_ptr)devs)->intr_num = irq;
	((tee_dev_t_ptr)devs)->apc_num = dapc_num;
	((tee_dev_t_ptr)devs)->param[0] = param0;

}

void CONFIG_ISEE_FLAGS(u64 *flags, u32 set_value, u64 mask, u32 offset)
{
	u64 teei_flags = *flags;
	teei_flags &= ~mask;
#ifdef TEEI_DEBUG
	IMSG_DBG("[START] FLAGS[0x%x] VALUE[0x%x] OFFSET[0x%x]\n", (unsigned int)teei_flags, (unsigned int)set_value, offset);
#endif
	teei_flags = teei_flags | (set_value << offset);
#ifdef TEEI_DEBUG
	IMSG_DBG("[END] FLAGS[0x%x]\n", (unsigned int)teei_flags);
#endif
	*flags = teei_flags;
}

void CONFIG_ISEE_FLAGS_MULTI_SELECTION(u64 *flags, u32 set_value, u64 mask, u32 offset)
{
	u64 teei_flags = *flags;

	if (!set_value) {
#ifdef TEEI_DEBUG
		IMSG_DBG("[START] FLAGS[0x%x] VALUE[0x%x] OFFSET[0x%x]\n", (unsigned int)teei_flags, (unsigned int)set_value, offset);
#endif
		teei_flags &= ~mask;

	}
	teei_flags |= (set_value << offset);
#ifdef TEEI_DEBUG
	IMSG_DBG("[END] FLAGS[0x%x]\n", (unsigned int)teei_flags);
#endif
	*flags = teei_flags;
}




#ifdef MTK_SEC_VIDEO_PATH_SUPPORT
#define RESERVED_SEC_MEMORY_SIZE_FOR_SECURE_MEMORY_MGR (3*1024*1024)    /* 3MB */
#define RESERVED_SEC_MEMORY_SIZE_FOR_RES1 (28*1024)                     /* 28KB */

void teei_secmem_param_prepare(u32 param_addr, u32 tee_entry,
    u32 teei_sec_dram_size, u32 tee_smem_size)
{
    int ret = 0;
    sec_mem_arg_t sec_mem_arg = {0};

    sec_mem_arg.magic = SEC_MEM_MAGIC;
    sec_mem_arg.version = SEC_MEM_VERSION;
    sec_mem_arg.svp_mem_start = tee_entry + teei_sec_dram_size -
        RESERVED_SEC_MEMORY_SIZE_FOR_SECURE_MEMORY_MGR;

#if CFG_TEE_SECURE_MEM_PROTECTED
    sec_mem_arg.secmem_obfuscation = 1;
#else
    sec_mem_arg.secmem_obfuscation = 0;
#endif

#if CFG_TEE_SECURE_MEM_SHARED
    /* Prepare secure memory configuration parameters */
    /* ------------------------------------------------------------------------------*/
    /* |  teei-core     | SVP sec mem  | RES1  | CMDQ  | M4U  | tplay table | tplay |*/
    /* |    60+33 MB    |   0KB        | 28 KB | 960KB | 32KB | tplay 4KB   |  2MB  |*/
    /* ------------------------------------------------------------------------------*/
    /* secure memory will not be occupied in the beginning */
    sec_mem_arg.shared_secmem = 1;
    /* 2MB for t-play (backward) */
    sec_mem_arg.tplay_mem_size = SEC_MEM_TPLAY_MEMORY_SIZE;
    sec_mem_arg.tplay_mem_start = tee_entry + (tee_smem_size - SEC_MEM_TPLAY_MEMORY_SIZE);
    /* 4KB for t-play table (backward) */
    sec_mem_arg.tplay_table_size = SEC_MEM_TPLAY_TABLE_SIZE;
    sec_mem_arg.tplay_table_start = sec_mem_arg.tplay_mem_start - SEC_MEM_TPLAY_TABLE_SIZE;
    /* 16KB for M4U (backward) */
    sec_mem_arg.m4u_mem_size = SEC_MEM_RESERVED_M4U_SIZE;
    sec_mem_arg.m4u_mem_start = sec_mem_arg.tplay_table_start - SEC_MEM_RESERVED_M4U_SIZE;
    /* 960KB for CMDQ (backward) */
    sec_mem_arg.cmdq_mem_size = SEC_MEM_RESERVED_CMDQ_SIZE;
    sec_mem_arg.cmdq_mem_start = sec_mem_arg.m4u_mem_start - SEC_MEM_RESERVED_CMDQ_SIZE;
    /* 28KB for RES1 (backward) */
    sec_mem_arg.spi_mem_size = RESERVED_SEC_MEMORY_SIZE_FOR_RES1;
    sec_mem_arg.spi_mem_start = sec_mem_arg.cmdq_mem_start  - RESERVED_SEC_MEMORY_SIZE_FOR_RES1;
    /* SVP region secure memory is allocated at run time via CMA */
    sec_mem_arg.svp_mem_end = sec_mem_arg.svp_mem_start;
#else
    /* This case is currently not supported, SVP memory must be mamanged via CMA */
#error "Please enable CFG_TEE_SECURE_MEM_SHARED in cust_bldr.mk"
#endif

    IMSG_DBG("%s sec_mem_arg.magic: 0x%x\n", __func__, sec_mem_arg.magic);
    IMSG_DBG("%s sec_mem_arg.version: 0x%x\n", __func__, sec_mem_arg.version);
    IMSG_DBG("%s sec_mem_arg.svp_mem_start: 0x%x\n", __func__, sec_mem_arg.svp_mem_start);
    IMSG_DBG("%s sec_mem_arg.svp_mem_end: 0x%x\n", __func__, sec_mem_arg.svp_mem_end);
    IMSG_DBG("%s sec_mem_arg.tplay_mem_start: 0x%x\n", __func__, sec_mem_arg.tplay_mem_start);
    IMSG_DBG("%s sec_mem_arg.tplay_mem_size: 0x%x\n", __func__, sec_mem_arg.tplay_mem_size);
    IMSG_DBG("%s sec_mem_arg.tplay_table_start: 0x%x\n", __func__, sec_mem_arg.tplay_table_start);
    IMSG_DBG("%s sec_mem_arg.tplay_table_size: 0x%x\n", __func__, sec_mem_arg.tplay_table_size);
    IMSG_DBG("%s sec_mem_arg.secmem_obfuscation: 0x%x\n", __func__, sec_mem_arg.secmem_obfuscation);
    IMSG_DBG("%s tee_entry_addr: 0x%x\n", __func__, tee_entry);
    IMSG_DBG("%s tee_secmem_size: 0x%x\n", __func__, tee_smem_size-RESERVED_SEC_MEMORY_SIZE_FOR_SECURE_MEMORY_MGR);
    IMSG_DBG("%s sec_mem_arg.shared_secmem: 0x%x\n", __func__, sec_mem_arg.shared_secmem);
    IMSG_DBG("%s sec_mem_arg.m4u_mem_start: 0x%x\n", __func__, sec_mem_arg.m4u_mem_start);
    IMSG_DBG("%s sec_mem_arg.m4u_mem_size: 0x%x\n", __func__, sec_mem_arg.m4u_mem_size);
    IMSG_DBG("%s sec_mem_arg.cmdq_mem_start: 0x%x\n", __func__, sec_mem_arg.cmdq_mem_start);
    IMSG_DBG("%s sec_mem_arg.cmdq_mem_size: 0x%x\n", __func__, sec_mem_arg.cmdq_mem_size);

    memcpy((void*)param_addr, &sec_mem_arg, sizeof(sec_mem_arg_t));

    /* The maximum size of this structure is 256 bytes only */
    COMPILE_ASSERT(0x100 >= sizeof(sec_mem_arg_t));
}
#endif

#ifdef TEEI_DEBUG
static void teei_boot_param_dump(tee_arg_t_ptr teearg)
{
    IMSG_DBG("============================DUMP START=============================\n");
    IMSG_DBG("%s teearg : 0x%x\n", __func__, teearg);
    IMSG_DBG("%s atf_magic : 0x%x\n", __func__, teearg->magic);
    IMSG_DBG("%s length : 0x%x\n", __func__, teearg->length);
    IMSG_DBG("%s version : 0x%x\n", __func__, teearg->version);
    IMSG_DBG("%s secDRamBase : 0x%x\n", __func__, teearg->secDRamBase);
    IMSG_DBG("%s secDRamSize : 0x%x\n", __func__, teearg->secDRamSize);
    IMSG_DBG("%s gic_distributor_base : 0x%x\n", __func__, teearg->gic_distributor_base);
    IMSG_DBG("%s gic_cpuinterface_base : 0x%x\n", __func__,teearg->gic_cpuinterface_base);
    IMSG_DBG("%s gic_version : 0x%x\n", __func__, teearg->gic_version);
    IMSG_DBG("%s total_number_spi : 0x%x\n", __func__, teearg->total_number_spi);
    IMSG_DBG("%s teearg->ssiq_number[0] : 0x%x\n", __func__, teearg->ssiq_number[0]);
    IMSG_DBG("%s teearg->ssiq_number[1] : 0x%x\n", __func__, teearg->ssiq_number[1]);
    IMSG_DBG("%s teearg->ssiq_number[2] : 0x%x\n", __func__, teearg->ssiq_number[2]);
    IMSG_DBG("%s teearg->ssiq_number[3] : 0x%x\n", __func__, teearg->ssiq_number[3]);
    IMSG_DBG("%s teearg->ssiq_number[4] : 0x%x\n", __func__, teearg->ssiq_number[4]);
    IMSG_DBG("%s teearg->tee_dev[0].dev_type : 0x%x\n", __func__, (teearg->tee_dev[0]).dev_type);
    IMSG_DBG("%s teearg->tee_dev[0].base_addr : 0x%x\n", __func__, (teearg->tee_dev[0]).base_addr);
    IMSG_DBG("%s teearg->tee_dev[0].intr_num : 0x%x\n", __func__, (teearg->tee_dev[0]).intr_num);
    IMSG_DBG("%s teearg->tee_dev[0].apc_num : 0x%x\n", __func__, (teearg->tee_dev[0]).apc_num);
    IMSG_DBG("%s teearg->tee_dev[1].dev_type : 0x%x\n", __func__, teearg->tee_dev[1].dev_type);
    IMSG_DBG("%s teearg->tee_dev[1].base_addr : 0x%x\n", __func__, teearg->tee_dev[1].base_addr);
    IMSG_DBG("%s teearg->tee_dev[1].intr_num : 0x%x\n", __func__, teearg->tee_dev[1].intr_num);
    IMSG_DBG("%s teearg->tee_dev[1].apc_num : 0x%x\n", __func__, teearg->tee_dev[1].apc_num);
    IMSG_DBG("%s teearg->tee_dev[2].dev_type : 0x%x\n", __func__, teearg->tee_dev[2].dev_type);
    IMSG_DBG("%s teearg->tee_dev[2].base_addr : 0x%x\n", __func__, teearg->tee_dev[2].base_addr);
    IMSG_DBG("%s teearg->tee_dev[2].intr_num : 0x%x\n", __func__, teearg->tee_dev[2].intr_num);
    IMSG_DBG("%s teearg->tee_dev[2].apc_num : 0x%x\n", __func__, teearg->tee_dev[2].apc_num);
    IMSG_DBG("%s teearg->tee_dev[3].dev_type : 0x%x\n", __func__, teearg->tee_dev[3].dev_type);
    IMSG_DBG("%s teearg->tee_dev[3].base_addr : 0x%x\n", __func__, teearg->tee_dev[3].base_addr);
    IMSG_DBG("%s teearg->tee_dev[3].intr_num : 0x%x\n", __func__, teearg->tee_dev[3].intr_num);
    IMSG_DBG("%s teearg->tee_dev[3].apc_num : 0x%x\n", __func__, teearg->tee_dev[3].apc_num);
}
#endif
void teei_boot_param_prepare(u32 param_addr, u32 tee_entry,
                             u64 teei_sec_dram_size, u64 dram_base, u64 dram_size)
{
    tee_arg_t_ptr teearg = (tee_arg_t_ptr)param_addr;
    u32 teei_log_port = 0;

    //IMSG_DBG("==================================================================\n");
    IMSG_DBG("uTos PL VERSION [%s]\n",UTOS_VERSION);
    //IMSG_DBG("==================================================================\n");

    if (teearg == NULL) {
        IMSG_ERROR("teei boot param argument is NULL!\n");
        return;
    }

    /* Prepare TEE boot parameters */
    teearg->magic = TEEI_BOOTCFG_MAGIC;             /* TEEI magic number */
    teearg->length = sizeof(tee_arg_t);             /* TEEI argument block size */
    teearg->version = TEE_ARGUMENT_VERSION;
    teearg->secDRamBase = tee_entry;                /* Secure DRAM base address */
#ifdef MTK_SEC_VIDEO_PATH_SUPPORT
    /* In case if SVP is supported, we need to reserved some memory for secure memory */
    teearg->secDRamSize = teei_sec_dram_size -
        RESERVED_SEC_MEMORY_SIZE_FOR_SECURE_MEMORY_MGR;     /* Secure DRAM size */
	IMSG_DBG("%s secDRamSize: 0x%x\n", __func__, teearg->secDRamSize);
#else
    teearg->secDRamSize = teei_sec_dram_size;               /* Secure DRAM size */
#endif

    /* SSI Reserve */
    teearg->total_number_spi = 256;                 /* Platform Support total 256 SPIs and 32 PPIs */
    //teearg->ssiq_number[0] = (32 + 248);            /* Platform reserve SPI ID 248 for TEEI, which is ID 280 */
    teearg->ssiq_number[0] = TEE_FCH_IRQ_NO;            /* Platform reserve SPI ID 248 for TEEI, which is ID 280 */
    teearg->ssiq_number[1] = TEE_DRV_IRQ_NO;            /* Platform reserve SPI ID 248 for TEEI, which is ID 280 */
    teearg->ssiq_number[2] = TEE_REV_IRQ_NO;            /* Platform reserve SPI ID 248 for TEEI, which is ID 280 */
    teearg->ssiq_number[3] = TEE_DUMPER_IRQ_NO;

    /* GIC parameters */
    teearg->gic_distributor_base = GIC_BASE ;
    teearg->gic_cpuinterface_base = GIC_CPU;
    teearg->gic_version = GIC_VERSION ;

    /* Configure for ISEE flags */
    teearg->flags = ISEE_FLAGS_DEFAULT_VALUE;
    CONFIG_ISEE_FLAGS(&teearg->flags, ISEE_FLAGS_LOG_PATH_BUFFER, ISEE_FLAGS_LOG_PATH_MASK, ISEE_FLAGS_LOG_PATH_OFFSET);
#if defined(TARGET_BUILD_VARIANT_ENG) || defined(TARGET_BUILD_VARIANT_USERDEBUG)
    CONFIG_ISEE_FLAGS(&teearg->flags, ISEE_FLAGS_LOG_LEVEL_INFO, ISEE_FLAGS_LOG_LEVEL_MASK, ISEE_FLAGS_LOG_LEVEL_OFFSET);
#else
    CONFIG_ISEE_FLAGS(&teearg->flags, ISEE_FLAGS_LOG_LEVEL_WARNING, ISEE_FLAGS_LOG_LEVEL_MASK, ISEE_FLAGS_LOG_LEVEL_OFFSET);
#endif
    CONFIG_ISEE_FLAGS(&teearg->flags, ISEE_FLAGS_SOTER_TYPE_NORMAL, ISEE_FLAGS_SOTER_TYPE_MASK, ISEE_FLAGS_SOTER_TYPE_OFFSET);
    CONFIG_ISEE_FLAGS(&teearg->flags, ISEE_FLAGS_VERIFY_MODE_DISABLE, ISEE_FLAGS_VERIFY_MODE_MASK, ISEE_FLAGS_VERIFY_MODE_OFFSET);
#if CFG_TEE_VIRTUAL_RPMB_SUPPORT
    CONFIG_ISEE_FLAGS(&teearg->flags, ISEE_FLAGS_SECURE_STORAGE_FS, ISEE_FLAGS_SECURE_STORAGE_MASK, ISEE_FLAGS_SECURE_STORAGE_OFFSET);
#else
    CONFIG_ISEE_FLAGS(&teearg->flags, ISEE_FLAGS_SECURE_STORAGE_RPMB, ISEE_FLAGS_SECURE_STORAGE_MASK, ISEE_FLAGS_SECURE_STORAGE_OFFSET);
#endif

    CONFIG_ISEE_FLAGS_MULTI_SELECTION(&teearg->flags, ISEE_FLAGS_SOTER_FEATURE_RESET, ISEE_FLAGS_SOTER_FEATURE_MASK, ISEE_FLAGS_SOTER_FEATURE_OFFSET);
#ifdef CFG_TEE_TUI_SUPPORT
    CONFIG_ISEE_FLAGS_MULTI_SELECTION(&teearg->flags, ISEE_FLAGS_SOTER_FEATURE_TUI, ISEE_FLAGS_SOTER_FEATURE_MASK, ISEE_FLAGS_SOTER_FEATURE_OFFSET);
#endif
#ifdef MTK_SEC_VIDEO_PATH_SUPPORT
    CONFIG_ISEE_FLAGS_MULTI_SELECTION(&teearg->flags, ISEE_FLAGS_SOTER_FEATURE_DRM, ISEE_FLAGS_SOTER_FEATURE_MASK, ISEE_FLAGS_SOTER_FEATURE_OFFSET);
#endif
 CONFIG_ISEE_FLAGS(&teearg->flags, ISEE_FLAGS_SOTER_SYSTEM_DEBUG_OPEN, ISEE_FLAGS_SOTER_SYSTEM_DEBUG_MASK, ISEE_FLAGS_SOTER_SYSTEM_DEBUG_OFFSET);

    IMSG_DBG("%s teearg->flags :  0x%x\n", __func__, (unsigned int)teearg->flags);

#if CFG_BOOT_ARGUMENT_BY_ATAG
    teei_log_port = g_uart;
    IMSG_DBG("%s teearg : CFG_BOOT_ARGUMENT_BY_ATAG 0x%x\n", __func__, g_uart);
#elif CFG_BOOT_ARGUMENT && !CFG_BOOT_ARGUMENT_BY_ATAG
    teei_log_port = bootarg.log_port;
    IMSG_DBG("%s teearg : CFG_BOOT_ARGUMENT 0x%x\n", __func__, bootarg.log_port);
#else
    teei_log_port = CFG_UART_LOG;
    IMSG_DBG("%s teearg : log port by prj cfg 0x%x\n", __func__, CFG_UART_LOG);
#endif
    /* UART parameters for Log Debug from Uart */
    switch (teei_log_port) {
        case UART0_BASE:
            CONFIG_ISEE_DEVS(&(teearg->tee_dev[0]), MT_UART16550, UART0_BASE, MT_UART0_IRQ, MT_UART0_DAPC, CFG_LOG_BAUDRATE);
            break;
        case UART1_BASE:
            CONFIG_ISEE_DEVS(&(teearg->tee_dev[0]), MT_UART16550, UART1_BASE, MT_UART1_IRQ, MT_UART1_DAPC, CFG_LOG_BAUDRATE);
            break;
        case UART2_BASE:
            CONFIG_ISEE_DEVS(&(teearg->tee_dev[0]), MT_UART16550, UART2_BASE, MT_UART2_IRQ, MT_UART2_DAPC, CFG_LOG_BAUDRATE);
            break;
        default:
            break;
    }

    /* SEC GPTIMER parameters */
    CONFIG_ISEE_DEVS(&(teearg->tee_dev[1]), MT_SEC_GPT, MT_SEC_GPT_BASE, MT_SEC_GPT_IRQ, MT_SEC_GPT_DAPC, 0);
    /* SEC GPTWDT parameters */
    CONFIG_ISEE_DEVS(&(teearg->tee_dev[2]), MT_SEC_WDT, MT_SEC_WDT_BASE, MT_SEC_WDT_IRQ, MT_SEC_WDT_DAPC, 0);
    /* SEC CRYPTO parameters */
    CONFIG_ISEE_DEVS(&(teearg->tee_dev[3]), MT_SEC_CRYPTO, MT_SEC_CRYPTO_BASE, MT_SEC_CRYPTO_IRQ, MT_SEC_CRYPTO_DAPC, 0);
    /* If tee dev is NOT used, it should be set the "MT_UNUSED" flag */
    CONFIG_ISEE_DEVS(&(teearg->tee_dev[4]), MT_UNUSED, 0, 0, 0, 0);
#ifdef TEEI_DEBUG
    teei_boot_param_dump(teearg);
#endif
}

void teei_key_param_prepare(u32 param_addr, u8 *hwuid, u32 hwid_size, u8 *hrid, u32 hrid_size, u8 *rpmb_key)
{
    tee_keys_t_ptr keyarg = (tee_keys_t_ptr)param_addr;
    keyarg->magic = TEEI_BOOTCFG_MAGIC;
    memcpy(keyarg->rpmb_key,rpmb_key,KEY_LEN);

    keyarg->hwid_size = hwid_size;
    memcpy(keyarg->hw_id, hwuid, keyarg->hwid_size);
    keyarg->hrid_size =  hrid_size; 				/* add hrid size atf hrid size */
    memcpy(keyarg->hr_id, hrid, keyarg->hrid_size);	/* add hrid copy */

#if 0
    int i;
    IMSG_SENST(">>> rpmb_key>>>:\n");
    for (i = 0; i < 32; i++) {
        IMSG_SENST_DATA("0x%x%s", rpmb_key[i], ((i+1)%16)?(" "):("\n"));
    }
#endif
}

#define         GPTEE_RPMB_BLOCK_NUM    5696
#define         GPTEE_RPMB_OFFSET         20
#define         DRM_RPMB_BLOCK_NUM      14048
#define         DRM_RPMB_OFFSET         4

extern unsigned long rtcgettime_teei(void);
void teei_sgpt1_init(u32 base_addr);

void teei_rtctime_param_prepare(u32 param_addr)
{
	tee_arg_t_ptr teearg = (tee_arg_t_ptr)param_addr;
	tee_dev_t_ptr teedev1 = &(teearg->tee_dev[1]);
	u32 rtctime = 0;
	rtctime = rtcgettime_teei();
	teedev1->param[0] = rtctime;
	teei_sgpt1_init(MT_SEC_GPT_BASE);
}

#define        SEC_APXGPT1_CON               0x220
#define        SEC_APXGPT1_PRESCALE          0x224
#define        SEC_APXGPT1_COUNTER_L         0x228
#define        SEC_APXGPT1_COUNTER_H         0x22C


void teei_sgpt1_init(u32 base_addr)
{
       unsigned int val;
        val = 0x0;
       __raw_writel(val, base_addr+SEC_APXGPT1_CON);   //disable

       val = 0x2;
        __raw_writel(val, base_addr+SEC_APXGPT1_CON);  //clears counter to 0

       val = 0x0;
        __raw_writel(val, base_addr+SEC_APXGPT1_PRESCALE);   //13M clock, one division

       val = 0x30;
        __raw_writel(val, base_addr+SEC_APXGPT1_CON);   //free run

       val = 0x31;
        __raw_writel(val, base_addr+SEC_APXGPT1_CON);   //enable

}
