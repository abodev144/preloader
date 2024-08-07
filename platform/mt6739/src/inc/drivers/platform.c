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

#include "typedefs.h"
#include "platform.h"
#include "boot_device.h"
#include "nand.h"
#include "mmc_common_inter.h"

#include "uart.h"
#include "nand_core.h"
#include "pll.h"
#include "i2c.h"
#include "rtc.h"
#include "emi.h"
#include "pmic.h"
#include "wdt.h"
#include "ram_console.h"
#include "cust_sec_ctrl.h"
#include "gpio.h"
#include "pmic_wrap_init.h"
#include "keypad.h"
#include "usbphy.h"
#include "timer.h"
#include "dram_buffer.h"
/*#include "spm_mtcmos.h"*/
#if defined(MTK_PTP)
#include "mt_ptp.h"
#endif //defined(MTK_PTP)
#include <clkbuf_ctl.h>
#include <pmic_drv.h>
#include "pl_version.h"

#ifdef MTK_TRNG_CALIBRATION
#if defined MTK_TRNG_CALIBRATION_1ST_ROUND
#define TRNG_CALIB_BUF_SIZE  (28 + 24 * 1066)
unsigned int __NOBITS_SECTION__(.secbuf) g_trng_buf[TRNG_CALIB_BUF_SIZE / sizeof(unsigned int)];
extern unsigned int trng_calib_1st_round(unsigned int *buf, unsigned int buf_sz);
#endif //MTK_TRNG_CALIBRATION_1ST_ROUND
#if defined  MTK_TRNG_CALIBRATION_2ND_ROUND
#define TRNG_CALIB_BUF_SIZE  (28 + 24 * 546135)
unsigned int *g_trng_buf = 0x42000000;
extern unsigned int trng_calib_2nd_round(unsigned int *buf, unsigned int buf_sz);
#endif //MTK_TRNG_CALIBRATION_2ND_ROUND
#endif //MTK_TRNG_CALIBRATION

/*============================================================================*/
/* CONSTAND DEFINITIONS                                                       */
/*============================================================================*/
#define MOD "[PLFM]"

/* part_hdr_t g_hdr; */
/*============================================================================*/
/* GLOBAL VARIABLES                                                           */
/*============================================================================*/
unsigned int __SRAM__ sys_stack[CFG_SYS_STACK_SZ >> 2];
const unsigned int sys_stack_sz = CFG_SYS_STACK_SZ;
boot_mode_t g_boot_mode;
boot_dev_t g_boot_dev;
meta_com_t g_meta_com_type = META_UNKNOWN_COM;
u32 g_meta_com_id = 0;
boot_reason_t g_boot_reason;
ulong g_boot_time;
u32 g_ddr_reserve_enable = 0;
u32 g_ddr_reserve_success = 0;
u32 g_ddr_reserve_ready = 0;
u32 g_ddr_reserve_ta_err = 0;
extern unsigned int part_num;
extern part_hdr_t   part_info[PART_MAX_NUM];
extern bool ram_console_is_abnormal_boot(void);

#if defined (MTK_KERNEL_POWER_OFF_CHARGING)
kal_bool kpoc_flag = false;
#endif

#if CFG_BOOT_ARGUMENT
#define bootarg g_dram_buf->bootarg
#endif

#if CFG_USB_AUTO_DETECT
bool g_usbdl_flag;
#endif

#if defined(MTK_AB_OTA_UPDATER)
boot_mode_t ab_boot_mode;
#endif

/*============================================================================*/
/* platform_core_handler                                                      */
/*============================================================================*/
void platform_core_handler(void)
{
    int i;

#if CFG_LOAD_SLT_AARCH64_KERNEL
    if (0 == aarch64_slt_done())
    {

        print("\n%s in platform_core_handler\n", MOD);
	return ;
    }
#endif
}

/*============================================================================*/
/* EXTERNAL FUNCTIONS                                                         */
/*============================================================================*/
static u32 boot_device_init(void)
{
    #if (CFG_BOOT_DEV == BOOTDEV_UFS)
    return (u32)ufs_init_device();
    #elif (CFG_BOOT_DEV == BOOTDEV_SDMMC)
    return (u32)mmc_init_device();
    #else
    return (u32)nand_init_device();
    #endif
}

int usb_accessory_in(void)
{
#if !CFG_FPGA_PLATFORM
    int exist = 0;

    if (PMIC_CHRDET_EXIST == pmic_IsUsbCableIn()) {
        exist = 1;
        #if !CFG_USBIF_COMPLIANCE
        /* enable charging current as early as possible to avoid can't enter
         * following battery charging flow when low battery
         */
        platform_set_chrg_cur(450);
        #endif
    }
    return exist;
#else
    return 1;
#endif
}

extern bool is_uart_cable_inserted(void);

int usb_cable_in(void)
{
#if !CFG_FPGA_PLATFORM
    int exist = 0;
    CHARGER_TYPE ret;

    if ((g_boot_reason == BR_USB) || usb_accessory_in()) {
        ret = mt_charger_type_detection();
        if (ret == STANDARD_HOST || ret == CHARGING_HOST) {
            print("\n%s USB cable in\n", MOD);
            mt_usb_phy_poweron();
            mt_usb_phy_savecurrent();

            exist = 1;
        } else if (ret == NONSTANDARD_CHARGER || ret == STANDARD_CHARGER) {
            #if CFG_USBIF_COMPLIANCE
            platform_set_chrg_cur(450);
            #endif
        }
    }

    return exist;
#else
    print("\n%s USB cable in\n", MOD);
    mt_usb_phy_poweron();
    mt_usb_phy_savecurrent();

    return 1;
#endif
}

#if CFG_FPGA_PLATFORM
void show_tx(void)
{
	UINT8 var;
	USBPHY_I2C_READ8(0x6E, &var);
	UINT8 var2 = (var >> 3) & ~0xFE;
	print("[USB]addr: 0x6E (TX), value: %x - %x\n", var, var2);
}

void store_tx(UINT8 value)
{
	UINT8 var;
	UINT8 var2;
	USBPHY_I2C_READ8(0x6E, &var);

	if (value == 0) {
		var2 = var & ~(1 << 3);
	} else {
		var2 = var | (1 << 3);
	}

	USBPHY_I2C_WRITE8(0x6E, var2);
	USBPHY_I2C_READ8(0x6E, &var);
	var2 = (var >> 3) & ~0xFE;

	print("[USB]addr: 0x6E TX [AFTER WRITE], value after: %x - %x\n", var, var2);
}

void show_rx(void)
{
	UINT8 var;
	USBPHY_I2C_READ8(0x77, &var);
	UINT8 var2 = (var >> 7) & ~0xFE;
	print("[USB]addr: 0x77 (RX) [AFTER WRITE], value after: %x - %x\n", var, var2);
}

void test_uart(void)
{
	int i=0;
	UINT8 val = 0;
	for (i=0; i<1000; i++)
	{
			show_tx();
            mdelay(300);
			if (val) {
				val = 0;
			}
			else {
				val = 1;
			}
			store_tx(val);
			show_rx();
            mdelay(1000);
	}
}
#endif

#if 0 //Temporarily disable to save code size.
void set_to_usb_mode(void)
{
		UINT8 var;
#if !CFG_FPGA_PLATFORM
		/* Turn on USB MCU Bus Clock */
		var = READ_REG(PERI_GLOBALCON_PDN0_SET);
		print("\n[USB]USB bus clock: 0x008, value: %x\n", var);
		USB_CLR_BIT(USB0_PDN, PERI_GLOBALCON_PDN0_SET);
		var = READ_REG(PERI_GLOBALCON_PDN0_SET);
		print("\n[USB]USB bus clock: 0x008, value after: %x\n", var);

		/* Switch from BC1.1 mode to USB mode */
		var = USBPHY_READ8(0x1A);
		print("\n[USB]addr: 0x1A, value: %x\n", var);
		USBPHY_WRITE8(0x1A, var & 0x7f);
		print("\n[USB]addr: 0x1A, value after: %x\n", USBPHY_READ8(0x1A));

		/* Set RG_UART_EN to 0 */
		var = USBPHY_READ8(0x6E);
		print("\n[USB]addr: 0x6E, value: %x\n", var);
		USBPHY_WRITE8(0x6E, var & ~0x01);
		print("\n[USB]addr: 0x6E, value after: %x\n", USBPHY_READ8(0x6E));

		/* Set RG_USB20_DM_100K_EN to 0 */
		var = USBPHY_READ8(0x22);
		print("\n[USB]addr: 0x22, value: %x\n", var);
		USBPHY_WRITE8(0x22, var & ~0x02);
		print("\n[USB]addr: 0x22, value after: %x\n", USBPHY_READ8(0x22));
#else
		/* Set RG_UART_EN to 0 */
		USBPHY_I2C_READ8(0x6E, &var);
		print("\n[USB]addr: 0x6E, value: %x\n", var);
		USBPHY_I2C_WRITE8(0x6E, var & ~0x01);
		USBPHY_I2C_READ8(0x6E, &var);
		print("\n[USB]addr: 0x6E, value after: %x\n", var);

		/* Set RG_USB20_DM_100K_EN to 0 */
		USBPHY_I2C_READ8(0x22, &var);
		print("\n[USB]addr: 0x22, value: %x\n", var);
		USBPHY_I2C_WRITE8(0x22, var & ~0x02);
		USBPHY_I2C_READ8(0x22, &var);
		print("\n[USB]addr: 0x22, value after: %x\n", var);
#endif
		var = READ_REG(UART1_BASE + 0x90);
		print("\n[USB]addr: 0x11002090 (UART1), value: %x\n", var);
		WRITE_REG(var & ~0x01, UART1_BASE + 0x90);
		print("\n[USB]addr: 0x11002090 (UART1), value after: %x\n", READ_REG(UART1_BASE + 0x90));
}
#endif

void set_to_uart_mode(void)
{
#if !CFG_FPGA_PLATFORM
		UINT32 var;

		/* RG_USB20_BC11_SW_EN 0x11F4_0818[23] = 1'b0 */
		USBPHY_CLR32(0x18, (0x1 << 23));

		/* Set RG_SUSPENDM 0x11F4_0868[3] to 1 */
		USBPHY_SET32(0x68, (0x1 << 3));

		/* force suspendm 0x11F4_0868[18] = 1 */
		USBPHY_SET32(0x68, (0x1 << 18));
		/* Set rg_uart_mode 0x11F4_0868[31:30] to 2'b01 */
		USBPHY_CLR32(0x68, (0x3 << 30));
		USBPHY_SET32(0x68, (0x1 << 30));

		/* force_uart_i 0x11F4_0868[29] = 0 */
		USBPHY_CLR32(0x68, (0x1 << 29));

		/* force_uart_bias_en 0X11f4_0868[28] = 1 */
		USBPHY_SET32(0x68, (0x1 << 28));

		/* force_uart_tx_oe 0x11F4_0868[27] = 1 */
		USBPHY_SET32(0x68, (0x1 << 27));

		/* force_uart_en 0x11F4_0868[26] = 1 */
		USBPHY_SET32(0x68, (0x1 << 26));

		/* RG_UART_BIAS_EN 0x11F4_086C[18] = 1 */
		USBPHY_SET32(0x6C, (0x1 << 18));

		/* RG_UART_TX_OE 0X11f4_086c[17] = 1 */
		USBPHY_SET32(0x6C, (0x1 << 17));

		/* Set RG_UART_EN to 1 */
		USBPHY_SET32(0x6C, (0x1 << 16));

		/* Set RG_USB20_DM_100K_EN to 1 */
		USBPHY_SET32(0x20, (0x1 << 17));

		/* GPIO Selection */
		var = readl(GPIO_BASE+RG_GPIO_SELECT);
		writel(var & (~(GPIO_SEL_MASK)), GPIO_BASE+RG_GPIO_SELECT);

		var = readl(GPIO_BASE+RG_GPIO_SELECT);
		writel(var | GPIO_SEL_UART0, GPIO_BASE+RG_GPIO_SELECT);

		var = readl(GPIO_BASE+RG_GPIO_SELECT);
		print("\n[USB] Switch to UART mode: 0x%X!!\n", var);
#else
		UINT8 var;

		/* Set ru_uart_mode to 2'b01 */
		USBPHY_I2C_READ8(0x6B, &var);
		printf("\n[USB]addr: 0x6B, value: %x\n", var);
		USBPHY_I2C_WRITE8(0x6B, var | 0x7C);
		USBPHY_I2C_READ8(0x6B, &var);
		printf("\n[USB]addr: 0x6B, value after: %x\n", var);

		/* Set RG_UART_EN to 1 */
		USBPHY_I2C_READ8(0x6E, &var);
		printf("\n[USB]addr: 0x6E, value: %x\n", var);
		USBPHY_I2C_WRITE8(0x6E, var | 0x07);
		USBPHY_I2C_READ8(0x6E, &var);
		printf("\n[USB]addr: 0x6E, value after: %x\n", var);

		/* Set RG_USB20_DM_100K_EN to 1 */
		USBPHY_I2C_READ8(0x22, &var);
		printf("\n[USB]addr: 0x22, value: %x\n", var);
		USBPHY_I2C_WRITE8(0x22, var | 0x02);
		USBPHY_I2C_READ8(0x22, &var);
		printf("\n[USB]addr: 0x22, value after: %x\n", var);
#endif
}

extern U32 PMIC_VUSB_EN(void);

void platform_vusb_on(void)
{
#if !CFG_FPGA_PLATFORM

    U32 ret = 0;

	ret = PMIC_VUSB_EN();

	if (ret == 0)
		print("[platform_vusb_on] VUSB33 is on\n");
	else
		print("[platform_vusb_on] Failed to turn on VUSB33!\n");
#endif
    return;
}

static void setup_lastpc(void)
{
#define DFD_SRAM               ((u64 *)0x300000)
#define LASTPC_BASE		(0x10220410)
#define DFD_NO_DUMP_MAGIC_OFFSET	(40)
#define WDT_STATUS		(0x1000700c)

	int i = 0;
	int cluster = 0;
	int cpu_in_cluster = 0;
	u64 pc_value_h, fp_value_h, sp_value_h;
	u64 pc_value, fp_value, sp_value;
	u32 mcu_base = LASTPC_BASE;
	char *dfd_no_dump_magic = (char *)(DFD_SRAM+DFD_NO_DUMP_MAGIC_OFFSET);

	/* if DFD did not dump data to SRAM, the magic string will keep
	 * or, if cold boot, in these 2 cases, we need to follow legacy lastpc flow */
	if ((dfd_no_dump_magic[0] == 'm' &&
		dfd_no_dump_magic[1] == 't' &&
		dfd_no_dump_magic[2] == '6' &&
		dfd_no_dump_magic[3] == '7' &&
		dfd_no_dump_magic[4] == '9' &&
		dfd_no_dump_magic[5] == '7' &&
		dfd_no_dump_magic[6] == 'D' &&
		dfd_no_dump_magic[7] == 'F' &&
		dfd_no_dump_magic[8] == 'D') || (g_rgu_status == 0)) {

		memset(DFD_SRAM, 0x0, sizeof(u64)*30);

		/* get 8 core's lastpc */
		for (i = 0; i < 8; i++) {
			cluster = i / 4;
			cpu_in_cluster = i % 4;
			pc_value_h =
				DRV_Reg32((mcu_base + 0x4) + (cpu_in_cluster << 5) + (0x100 * cluster));
			pc_value =
				(pc_value_h << 32) |
				DRV_Reg32((mcu_base + 0x0) + (cpu_in_cluster << 5) + (0x100 * cluster));
			fp_value_h =
				DRV_Reg32((mcu_base + 0x14) + (cpu_in_cluster << 5) + (0x100 * cluster));
			fp_value =
				(fp_value_h << 32) |
				DRV_Reg32((mcu_base + 0x10) + (cpu_in_cluster << 5) + (0x100 * cluster));
			sp_value_h =
				DRV_Reg32((mcu_base + 0x1c) + (cpu_in_cluster << 5) + (0x100 * cluster));
			sp_value =
				(sp_value_h << 32) |
				DRV_Reg32((mcu_base + 0x18) + (cpu_in_cluster << 5) + (0x100 * cluster));

			*(DFD_SRAM+i) = pc_value;
			*(DFD_SRAM+i+10) = fp_value;
			*(DFD_SRAM+i+20) = sp_value;
		}
	}
}

#if CFG_BOOT_ARGUMENT_BY_ATAG
void platform_set_boot_args_by_atag(unsigned *ptr)
{
  int i=0, j=0;
    boot_tag *tags;

    tags = ptr;
    tags->hdr.size = boot_tag_size(boot_tag_boot_reason);
    tags->hdr.tag = BOOT_TAG_BOOT_REASON;
    tags->u.boot_reason.boot_reason = g_boot_reason;
    ptr += tags->hdr.size;

    tags = ptr;
    tags->hdr.size = boot_tag_size(boot_tag_boot_mode);
    tags->hdr.tag = BOOT_TAG_BOOT_MODE;
    tags->u.boot_mode.boot_mode = g_boot_mode;
    ptr += tags->hdr.size;

    tags = ptr;
    tags->hdr.size = boot_tag_size(boot_tag_meta_com);
    tags->hdr.tag = BOOT_TAG_META_COM;
    tags->u.meta_com.meta_com_type = (u32)g_meta_com_type;
    tags->u.meta_com.meta_com_id = g_meta_com_id;
    tags->u.meta_com.meta_uart_port = CFG_UART_META;
    ptr += tags->hdr.size;

    tags = ptr;
    tags->hdr.size = boot_tag_size(boot_tag_log_com);
    tags->hdr.tag = BOOT_TAG_LOG_COM;
    tags->u.log_com.log_port = CFG_UART_LOG;
#if CFG_TEE_SUPPORT
    // Switch log port to UART2 while uart meta connected.
    if (g_boot_mode == META_BOOT && g_meta_com_type == META_UART_COM)
        tags->u.log_com.log_port = UART2;
#endif
    tags->u.log_com.log_baudrate = CFG_LOG_BAUDRATE;
    tags->u.log_com.log_enable = (u8)log_status();
#if CFG_UART_DYNAMIC_SWITCH
    tags->u.log_com.log_dynamic_switch = (u8)get_log_switch();
#endif
    ptr += tags->hdr.size;

  //---- DRAM tags ----
	mblock_set_mem_tag(&ptr);
  //----------------
    tags = ptr;
    tags->hdr.size = boot_tag_size(boot_tag_md_info);
    tags->hdr.tag = BOOT_TAG_MD_INFO;
    for(i=0;i<4;i++)
	tags->u.md_info.md_type[i] = bootarg.md_type[i];
    ptr += tags->hdr.size;

    tags = ptr;
    tags->hdr.size = boot_tag_size(boot_tag_boot_time);
    tags->hdr.tag = BOOT_TAG_BOOT_TIME;
    tags->u.boot_time.boot_time = get_timer(g_boot_time);;
    ptr += tags->hdr.size;

    tags = ptr;
    tags->hdr.size = boot_tag_size(boot_tag_da_info);
    tags->hdr.tag = BOOT_TAG_DA_INFO;
    tags->u.da_info.da_info.addr = bootarg.da_info.addr;
    tags->u.da_info.da_info.arg1 = bootarg.da_info.arg1;
    tags->u.da_info.da_info.arg2 = bootarg.da_info.arg2;
    tags->u.da_info.da_info.len  = bootarg.da_info.len;
    tags->u.da_info.da_info.sig_len = bootarg.da_info.sig_len;
    ptr += tags->hdr.size;

    tags = ptr;
    tags->hdr.size = boot_tag_size(boot_tag_sec_info);
    tags->hdr.tag = BOOT_TAG_SEC_INFO;
    tags->u.sec_info.sec_limit.magic_num = bootarg.sec_limit.magic_num;
    tags->u.sec_info.sec_limit.forbid_mode = bootarg.sec_limit.forbid_mode;
    ptr += tags->hdr.size;

    tags = ptr;
    tags->hdr.size = boot_tag_size(boot_tag_part_num);
    tags->hdr.tag = BOOT_TAG_PART_NUM;
    tags->u.part_num.part_num = g_dram_buf->part_num;
    ptr += tags->hdr.size;

    tags = ptr;
    tags->hdr.size = boot_tag_size(boot_tag_part_info);
    tags->hdr.tag = BOOT_TAG_PART_INFO;
    tags->u.part_info.part_info = g_dram_buf->part_info;
    ptr += tags->hdr.size;

    tags = ptr;
    tags->hdr.size = boot_tag_size(boot_tag_eflag);
    tags->hdr.tag = BOOT_TAG_EFLAG;
    tags->u.eflag.e_flag = sp_check_platform();
    ptr += tags->hdr.size;

    tags = ptr;
    tags->hdr.size = boot_tag_size(boot_tag_ddr_reserve);
    tags->hdr.tag = BOOT_TAG_DDR_RESERVE;
    tags->u.ddr_reserve.ddr_reserve_enable = g_ddr_reserve_enable;
    tags->u.ddr_reserve.ddr_reserve_success = (g_ddr_reserve_ta_err == 0) ? 1 : 0;
    tags->u.ddr_reserve.ddr_reserve_ready = g_ddr_reserve_ready;
    ptr += tags->hdr.size;

    tags = ptr;
    tags->hdr.size  = boot_tag_size(boot_tag_dram_buf);
    tags->hdr.tag   = BOOT_TAG_DRAM_BUF;
    tags->u.dram_buf.dram_buf_size = sizeof(dram_buf_t);
    ptr += tags->hdr.size;

    tags = ptr;
    tags->hdr.size = boot_tag_size(boot_tag_sram_info);
    tags->hdr.tag  = BOOT_TAG_SRAM_INFO;
    tags->u.sram_info.non_secure_sram_addr= CFG_NON_SECURE_SRAM_ADDR;
    tags->u.sram_info.non_secure_sram_size = CFG_NON_SECURE_SRAM_SIZE;
    ptr += tags->hdr.size;

#if defined(MTK_PTP)
    tags = ptr;
    tags->hdr.size  = boot_tag_size(boot_tag_ptp);
    tags->hdr.tag   = BOOT_TAG_PTP;
    mt_set_ptp_info(&bootarg.ptp_volt_info);
    tags->u.ptp_volt.ptp_volt_info.first_volt = bootarg.ptp_volt_info.first_volt;
    tags->u.ptp_volt.ptp_volt_info.second_volt = bootarg.ptp_volt_info.second_volt;
    tags->u.ptp_volt.ptp_volt_info.third_volt = bootarg.ptp_volt_info.third_volt;
    tags->u.ptp_volt.ptp_volt_info.have_550 = bootarg.ptp_volt_info.have_550;
    ptr += tags->hdr.size;
#endif //defined(MTK_PTP)
    tags = (boot_tag *)ptr;
    tags->hdr.size = boot_tag_size(boot_tag_is_abnormal_boot);
    tags->hdr.tag = BOOT_TAG_IS_ABNORMAL_BOOT;
    tags->u.is_abnormal_boot.is_abnormal_boot = ram_console_is_abnormal_boot();
    ptr += tags->hdr.size;

	tags = ptr;
	tags->hdr.size = boot_tag_size(boot_tag_emi_info);
	tags->hdr.tag = BOOT_TAG_EMI_INFO;
	tags->u.emi_info.dram_type = mt_get_dram_type();
	tags->u.emi_info.ch_num = 2;
	tags->u.emi_info.rk_num = 2;
    ptr += tags->hdr.size;

  /* END */
  *ptr++ = 0;
  *ptr++ = 0;
}

void platform_dump_boot_atag()
{
    int i=0, j=0;
    boot_tag *tags;
	for (tags = &(g_dram_buf->boottag); tags->hdr.size; tags = boot_tag_next(tags))
	{
        switch(tags->hdr.tag)
		{
            case BOOT_TAG_BOOT_REASON:
				print("BOOT_REASON: %d\n", tags->u.boot_reason.boot_reason);
				break;
			case BOOT_TAG_BOOT_MODE:
				print("BOOT_MODE: %d\n", tags->u.boot_mode.boot_mode);
				break;
			case BOOT_TAG_META_COM:
				print("META_COM TYPE: %d\n", tags->u.meta_com.meta_com_type);
				print("META_COM ID: %d\n", tags->u.meta_com.meta_com_id);
				print("META_COM PORT: %d\n", tags->u.meta_com.meta_uart_port);
				break;
			case BOOT_TAG_LOG_COM:
				print("LOG_COM PORT: %d\n", tags->u.log_com.log_port);
				print("LOG_COM BAUD: %d\n", tags->u.log_com.log_baudrate);
				print("LOG_COM EN: %d\n", tags->u.log_com.log_enable);
#if CFG_UART_DYNAMIC_SWITCH
				print("LOG_COM SWITCH: %d\n", tags->u.log_com.log_dynamic_switch);
#endif
				break;
			case BOOT_TAG_MEM:
				mblock_dump_mem_tag((unsigned int*)tags);
				break;
			case BOOT_TAG_MD_INFO:
				for(i=0;i<4;i++)
					print("MD_INFO: 0x%x\n", tags->u.md_info.md_type[i]);
				break;
			case BOOT_TAG_BOOT_TIME:
				print("BOOT_TIME: %d\n", tags->u.boot_time.boot_time);
				break;
			case BOOT_TAG_DA_INFO:
				print("DA_INFO: 0x%x\n", tags->u.da_info.da_info.addr);
				print("DA_INFO: 0x%x\n", tags->u.da_info.da_info.arg1);
				print("DA_INFO: 0x%x\n", tags->u.da_info.da_info.arg2);
				print("DA_INFO: 0x%x\n", tags->u.da_info.da_info.len);
				print("DA_INFO: 0x%x\n", tags->u.da_info.da_info.sig_len);
				break;
			case BOOT_TAG_SEC_INFO:
				print("SEC_INFO: 0x%x\n", tags->u.sec_info.sec_limit.magic_num);
				print("SEC_INFO: 0x%x\n", tags->u.sec_info.sec_limit.forbid_mode);
				break;
			case BOOT_TAG_PART_NUM:
				print("PART_NUM: %d\n", tags->u.part_num.part_num);
				break;
			case BOOT_TAG_PART_INFO:
				print("PART_INFO: 0x%x\n", tags->u.part_info.part_info);
				break;
			case BOOT_TAG_EFLAG:
				print("EFLAG: %d\n", tags->u.eflag.e_flag);
				break;
			case BOOT_TAG_DDR_RESERVE:
				print("DDR_RESERVE: %d\n", tags->u.ddr_reserve.ddr_reserve_enable);
				print("DDR_RESERVE: %d\n", tags->u.ddr_reserve.ddr_reserve_success);
				print("DDR_RESERVE: %d\n", tags->u.ddr_reserve.ddr_reserve_ready);
				break;
			case BOOT_TAG_DRAM_BUF:
				print("DRAM_BUF: %d\n", tags->u.dram_buf.dram_buf_size);
				break;
			case BOOT_TAG_SRAM_INFO:
				print("SRAM start: 0x%x\n", tags->u.sram_info.non_secure_sram_addr);
				print("SRAM size: 0x%x\n", tags->u.sram_info.non_secure_sram_size);
				break;
			case BOOT_TAG_IS_ABNORMAL_BOOT:
				print("is_abnormal_boot: %d\n", tags->u.is_abnormal_boot.is_abnormal_boot);
				break;
        }
	}
}
#endif

void platform_set_boot_args(void)
{
    int i;

#if CFG_ATF_SUPPORT
    u32 sec_addr, sec_size = 0;
    tee_get_secmem_start(&sec_addr);
    tee_get_secmem_size(&sec_size);
    bootarg.tee_reserved_mem.start = sec_addr;
    bootarg.tee_reserved_mem.size = sec_size;
#else
    bootarg.tee_reserved_mem.start = 0;
    bootarg.tee_reserved_mem.size = 0;
#endif

#if CFG_BOOT_ARGUMENT_BY_ATAG
    print("\n%s boot to LK by ATAG.\n", MOD, g_boot_reason);
    platform_set_boot_args_by_atag(&(g_dram_buf->boottag)); // set jump addr
    platform_dump_boot_atag();

#if CFG_TEE_SUPPORT
    // Switch log port to UART2 while uart meta connected.
    if (g_boot_mode == META_BOOT && g_meta_com_type == META_UART_COM)
    {
        print("%s UART META Connected, Switch log port to UART2...\n", MOD);
        mtk_serial_set_current_uart(UART2);
    }
#endif
#elif CFG_BOOT_ARGUMENT && !CFG_BOOT_ARGUMENT_BY_ATAG
    bootarg.magic = BOOT_ARGUMENT_MAGIC;
    bootarg.mode  = g_boot_mode;
    bootarg.e_flag = sp_check_platform();
    bootarg.log_port = CFG_UART_LOG;
    bootarg.log_baudrate = CFG_LOG_BAUDRATE;
    bootarg.log_enable = (u8)log_status();
#if !CFG_FPGA_PLATFORM
/*In FPGA phase, dram related function should by pass*/
    bootarg.dram_rank_num = bootarg.mblock_info.mblock_num;
    for (i = 0; i < bootarg.mblock_info.mblock_num; i++) {
	    bootarg.dram_rank_size[i] = bootarg.mblock_info.mblock[i].size;
        print("%s, rank[%d].size = 0x%llx\n", __func__, i,
			(unsigned long long) bootarg.dram_rank_size[i]);
    }
#endif
    bootarg.boot_reason = g_boot_reason;
    bootarg.meta_com_type = (u32)g_meta_com_type;
    bootarg.meta_com_id = g_meta_com_id;
    bootarg.meta_uart_port = CFG_UART_META;
    bootarg.boot_time = get_timer(g_boot_time);

    bootarg.part_num =  g_dram_buf->part_num;
    bootarg.part_info = g_dram_buf->part_info;

    bootarg.ddr_reserve_enable = g_ddr_reserve_enable;
    bootarg.ddr_reserve_success = (g_ddr_reserve_ta_err == 0) ? 1 : 0;
    bootarg.ddr_reserve_ready = g_ddr_reserve_ready;
    bootarg.dram_buf_size =  sizeof(dram_buf_t);

#if defined(MTK_PTP)
	mt_set_ptp_info(&bootarg.ptp_volt_info);
#endif //defined(MTK_PTP)

    bootarg.non_secure_sram_addr = CFG_NON_SECURE_SRAM_ADDR;
    bootarg.non_secure_sram_size = CFG_NON_SECURE_SRAM_SIZE;

    print("%s NON SECURE SRAM ADDR: 0x%x\n", MOD, bootarg.non_secure_sram_addr);
    print("%s NON SECURE SRAM SIZE: 0x%x\n", MOD, bootarg.non_secure_sram_size);

#if CFG_WORLD_PHONE_SUPPORT
    print("%s md_type[0] = %d \n", MOD, bootarg.md_type[0]);
    print("%s md_type[1] = %d \n", MOD, bootarg.md_type[1]);
#endif

    print("\n%s boot reason: %d\n", MOD, g_boot_reason);
    print("%s boot mode: %d\n", MOD, g_boot_mode);
    print("%s META COM%d: %d\n", MOD, bootarg.meta_com_id, bootarg.meta_com_type);
    print("%s <0x%x>: 0x%x\n", MOD, &bootarg.e_flag, bootarg.e_flag);
    print("%s boot time: %dms\n", MOD, bootarg.boot_time);
    print("%s DDR reserve mode: enable = %d, success = %d, ready = %d\n", MOD, bootarg.ddr_reserve_enable, bootarg.ddr_reserve_success, bootarg.ddr_reserve_ready);
    print("%s dram_buf_size: 0x%x\n", MOD, bootarg.dram_buf_size);
    print("%s tee_reserved_mem: 0x%llx, 0x%llx\n", MOD, bootarg.tee_reserved_mem.start, bootarg.tee_reserved_mem.size);
#if CFG_TEE_SUPPORT
    // Switch log port to UART2 while uart meta connected.
    if (g_boot_mode == META_BOOT && g_meta_com_type == META_UART_COM)
    {
        print("%s UART META Connected, Switch log port to UART2...\n", MOD);
        bootarg.log_port = UART2;
        mtk_serial_set_current_uart(UART2);
    }
#endif
#endif

}

void platform_set_dl_boot_args(da_info_t *da_info)
{
#if CFG_BOOT_ARGUMENT
    if (da_info->addr != BA_FIELD_BYPASS_MAGIC)
	bootarg.da_info.addr = da_info->addr;

    if (da_info->arg1 != BA_FIELD_BYPASS_MAGIC)
	bootarg.da_info.arg1 = da_info->arg1;

    if (da_info->arg2 != BA_FIELD_BYPASS_MAGIC)
	bootarg.da_info.arg2 = da_info->arg2;

    if (da_info->len != BA_FIELD_BYPASS_MAGIC)
	bootarg.da_info.len = da_info->len;

    if (da_info->sig_len != BA_FIELD_BYPASS_MAGIC)
	bootarg.da_info.sig_len = da_info->sig_len;
#endif

    return;
}

void platform_wdt_all_kick(void)
{
    /* kick watchdog to avoid cpu reset */
    mtk_wdt_restart();

#if !CFG_FPGA_PLATFORM
    /* kick PMIC watchdog to keep charging */
    //remove empty function to save space pl_kick_chr_wdt();
#endif
}

void platform_wdt_kick(void)
{
    /* kick hardware watchdog */
    mtk_wdt_restart();
}

#if CFG_DT_MD_DOWNLOAD
void platform_modem_download(void)
{
    print("[%s] modem download...\n", MOD);

    /* Switch to MT6261 USB:
     * GPIO_USB_SW1(USB_SW1)=1		//GPIO115, GPIO48
     * GPIO_USB_SW2(USB_SW2)=0		//GPIO116, GPIO196
     * phone
    #define GPIO_UART_URTS0_PIN         (GPIO115 | 0x80000000)
    #define GPIO_UART_UCTS0_PIN         (GPIO116 | 0x80000000)
     * EVB
    #define GPIO_EXT_USB_SW1         (GPIO48 | 0x80000000)
    #define GPIO_EXT_USB_SW2         (GPIO196 | 0x80000000)
     */
#if defined(GPIO_EXT_USB_SW1)
    mt_set_gpio_mode(GPIO_EXT_USB_SW1, GPIO_EXT_USB_SW1_M_GPIO);
    mt_set_gpio_dir(GPIO_EXT_USB_SW1, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_EXT_USB_SW1, GPIO_OUT_ONE);
#endif

#if defined(GPIO_EXT_USB_SW2)
	mt_set_gpio_mode(GPIO_EXT_USB_SW2, GPIO_EXT_USB_SW2_M_GPIO);
    mt_set_gpio_dir(GPIO_EXT_USB_SW2, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_EXT_USB_SW2, GPIO_OUT_ZERO);
#endif

    /* Press MT6261 DL key to enter download mode
     * GPIO_KPD_KROW1_PIN(GPIO_KCOL0)=0 //GPIO120, GPIO105
     * phone
    #define GPIO_KPD_KROW1_PIN         (GPIO120 | 0x80000000)
     * evb
    #define GPIO_EXT_MD_DL_KEY         (GPIO105 | 0x80000000)
     *
     */
#if defined(GPIO_EXT_MD_DL_KEY)
    mt_set_gpio_mode(GPIO_EXT_MD_DL_KEY, GPIO_EXT_MD_DL_KEY_M_GPIO);
    mt_set_gpio_dir(GPIO_EXT_MD_DL_KEY, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_EXT_MD_DL_KEY, GPIO_OUT_ZERO);
#endif

    /* Bring-up MT6261:
     * GPIO_OTG_DRVVBUS_PIN(GPIO_USB_DRVVBUS)=0,
     * GPIO_52_RST(GPIO_RESETB)=INPUT/NOPULL, GPIO118, GPIO166
     #define GPIO_UART_UTXD3_PIN         (GPIO118 | 0x80000000)
     #define GPIO_EXT_MD_RST         (GPIO166 | 0x80000000)
     * GPIO_RST_KEY(GPIO_PWRKEY)=0->1->0
     #define GPIO_UART_URXD3_PIN         (GPIO117 | 0x80000000)
     * GPIO_PWR_KEY(GPIO_PWRKEY)=1
     #define GPIO_EXT_MD_PWR_KEY         (GPIO167 | 0x80000000)
     */
#if defined(GPIO_EXT_MD_RST)
//    mt_set_gpio_mode(GPIO_OTG_DRVVBUS_PIN, GPIO_OTG_DRVVBUS_PIN_M_GPIO);
    mt_set_gpio_mode(GPIO_EXT_MD_RST, GPIO_EXT_MD_RST_M_GPIO);
#endif

    /* MD DRVVBUS to low */
//    mt_set_gpio_dir(GPIO_OTG_DRVVBUS_PIN, GPIO_DIR_OUT);
//    mt_set_gpio_out(GPIO_OTG_DRVVBUS_PIN, GPIO_OUT_ZERO);
#if defined(GPIO_EXT_MD_PWR_KEY)
    mt_set_gpio_mode(GPIO_EXT_MD_PWR_KEY, GPIO_EXT_MD_PWR_KEY_M_GPIO);
    mt_set_gpio_dir(GPIO_EXT_MD_PWR_KEY, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_EXT_MD_PWR_KEY, GPIO_OUT_ONE); /* default @ reset state */
#endif

#if defined(GPIO_EXT_MD_RST)
    /* MD reset pin: hold to zero */
    mt_set_gpio_pull_enable(GPIO_EXT_MD_RST, GPIO_PULL_DISABLE);
    mt_set_gpio_dir(GPIO_EXT_MD_RST, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_EXT_MD_RST, GPIO_OUT_ZERO); /* default @ reset state */
#endif
    mdelay(200);


    /* MD reset pin: released */
#if defined(GPIO_EXT_MD_RST)
    mt_set_gpio_out(GPIO_EXT_MD_RST, GPIO_OUT_ONE);
    mdelay(200);
    mt_set_gpio_dir(GPIO_EXT_MD_RST, GPIO_DIR_IN);
#endif

    print("[%s] AP modem download done\n", MOD);
	//keep kick WDT to avoid HW WDT reset
    while (1) {
        platform_wdt_all_kick();
		mdelay(1000);
    }
}
#endif

#if CFG_USB_AUTO_DETECT
void platform_usbdl_flag_check()
{
    U32 usbdlreg = 0;
    usbdlreg = DRV_Reg32(USBDL_FLAG);
     /*Set global variable to record the usbdl flag*/
    if(usbdlreg & USBDL_BIT_EN)
        g_usbdl_flag = 1;
    else
        g_usbdl_flag = 0;
}

void platform_usb_auto_detect_flow()
{

    print("USB DL Flag is %d when enter preloader  \n",g_usbdl_flag);

    /*usb download flag haven't set */
	if(g_usbdl_flag == 0 && g_boot_reason != BR_RTC){
        /*set up usbdl flag*/
        platform_safe_mode(1,CFG_USB_AUTO_DETECT_TIMEOUT_MS);
        print("Preloader going reset and trigger BROM usb auto detectiton!!\n");

        /*WDT by pass powerkey reboot*/
        //keep the previous status, pass it into reset function
        if(WDT_BY_PASS_PWK_REBOOT == mtk_wdt_boot_check())
            mtk_arch_reset(1);
        else
            mtk_arch_reset(0);

	}else{
    /*usb download flag have been set*/
    }

}
#endif


void platform_safe_mode(int en, u32 timeout)
{
#if !CFG_FPGA_PLATFORM

    U32 usbdlreg = 0;

    /* if anything is wrong and caused wdt reset, enter bootrom download mode */
    timeout = !timeout ? USBDL_TIMEOUT_MAX : timeout / 1000;
    timeout <<= 2;
    timeout &= USBDL_TIMEOUT_MASK; /* usbdl timeout cannot exceed max value */

    usbdlreg |= timeout;
    if (en)
	    usbdlreg |= USBDL_BIT_EN;
    else
	    usbdlreg &= ~USBDL_BIT_EN;

    usbdlreg &= ~USBDL_BROM ;
    /*Add magic number for MT6582*/
    usbdlreg |= USBDL_MAGIC;

    // set BOOT_MISC0 as watchdog resettable
    DRV_WriteReg32(MISC_LOCK_KEY,MISC_LOCK_KEY_MAGIC);
    DRV_SetReg32(RST_CON,1);
    DRV_WriteReg32(MISC_LOCK_KEY,0);

    DRV_WriteReg32(USBDL_FLAG,usbdlreg);

    return;
#endif

}

#if CFG_EMERGENCY_DL_SUPPORT
void platform_emergency_download(u32 timeout)
{
    /* enter download mode */
    print("%s emergency download mode(timeout: %ds).\n", MOD, timeout / 1000);
    platform_safe_mode(1, timeout);

#if !CFG_FPGA_PLATFORM
    mtk_arch_reset(0); /* don't bypass power key */
#endif

    while(1);
}
#endif

int platform_get_mcp_id(u8 *id, u32 len, u32 *fw_id_len)
{
    int ret = -1;

    memset(id, 0, len);

#if (CFG_BOOT_DEV == BOOTDEV_UFS)
    ret = ufs_get_device_id(id, len, fw_id_len);
#elif (CFG_BOOT_DEV == BOOTDEV_SDMMC)
    ret = mmc_get_device_id(id, len,fw_id_len);
#else
    ret = nand_get_device_id(id, len);
#endif

    return ret;
}

void platform_set_chrg_cur(int ma)
{
    //remove empty function to save space hw_set_cc(ma);
}

static boot_reason_t platform_boot_status(void)
{
#if !CFG_FPGA_PLATFORM
#if defined (MTK_KERNEL_POWER_OFF_CHARGING)
	ulong begin = get_timer(0);
	do  {
		if (rtc_boot_check()) {
			print("%s RTC boot!\n", MOD);
			return BR_RTC;
		}
		if(!kpoc_flag)
			break;
	} while (get_timer(begin) < 1000 && kpoc_flag);
#else
    if (rtc_boot_check()) {
        print("%s RTC boot!\n", MOD);
        return BR_RTC;
    }

#endif
    BOOTING_TIME_PROFILING_LOG("check Boot status-RTC");
#endif

    if (mtk_wdt_boot_check() == WDT_NORMAL_REBOOT) {
        print("%s WDT normal boot!\n", MOD);
        return BR_WDT;
    } else if (mtk_wdt_boot_check() == WDT_BY_PASS_PWK_REBOOT){
        print("%s WDT reboot bypass power key!\n", MOD);
        return BR_WDT_BY_PASS_PWK;
    }
    BOOTING_TIME_PROFILING_LOG("check Boot status-WDT");

#if !CFG_FPGA_PLATFORM
    /* check power key */
    if ((mtk_detect_key(PL_PMIC_PWR_KEY) && hw_check_battery()) || is_pmic_long_press_reset()) {
        print("%s Power key boot!\n", MOD);
        rtc_mark_bypass_pwrkey();
        return BR_POWER_KEY;
    }
    BOOTING_TIME_PROFILING_LOG("check Boot status-PWR key");
#endif



#if !CFG_EVB_PLATFORM
    if (usb_accessory_in()) {
        print("%s USB/charger boot!\n", MOD);
        BOOTING_TIME_PROFILING_LOG("check Boot status-usb_accessory_in");
        return BR_USB;
    }
    //need to unlock rtc PROT
    //check after rtc_boot_check() initial finish.
	if (rtc_2sec_reboot_check()) {
		print("%s 2sec reboot!\n", MOD);
		BOOTING_TIME_PROFILING_LOG("check Boot status-rtc_2sec_reboot_check");
		return BR_2SEC_REBOOT;
	}

    print("%s Unknown boot!\n", MOD);
    pl_power_off();
    /* should nerver be reached */
#endif

    print("%s Power key boot!\n", MOD);

    return BR_POWER_KEY;
}

#if CFG_LOAD_DSP_ROM || CFG_LOAD_MD_ROM
int platform_is_three_g(void)
{
    u32 tmp = sp_check_platform();

    return (tmp & 0x1) ? 0 : 1;
}
#endif

chip_ver_t platform_chip_ver(void)
{
    chip_ver_t sw_ver;
    unsigned int ver;

    ver = DRV_Reg32(APHW_VER);
    if (0xCA00 == ver)
        sw_ver = CHIP_VER_E1;
    else  //0xCA01
        sw_ver = CHIP_VER_E2;

    return sw_ver;
}

// ------------------------------------------------
// detect download mode
// ------------------------------------------------

bool platform_com_wait_forever_check(void)
{
#ifdef USBDL_DETECT_VIA_KEY
    /* check download key */
    if (TRUE == mtk_detect_key(COM_WAIT_KEY)) {
        print("%s COM handshake timeout force disable: Key\n", MOD);
        return TRUE;
    }
#endif

#ifdef USBDL_DETECT_VIA_AT_COMMAND
    print("platform_com_wait_forever_check\n");
    /* check SRAMROM_USBDL_TO_DIS */
    if (USBDL_TO_DIS == (INREG32(SRAMROM_USBDL_TO_DIS) & USBDL_TO_DIS)) {
	print("%s COM handshake timeout force disable: AT Cmd\n", MOD);
	CLRREG32(SRAMROM_USBDL_TO_DIS, USBDL_TO_DIS);
	return TRUE;
    }
#endif

    return FALSE;
}

/* NOTICE: need to revise if platform supports >4G memory size*/
u64 platform_memory_size(void)
{
    static u64 mem_size = 0;
    int nr_bank;
    int i;
    u64 rank_size[4], *rksize = &rank_size[0];
    u64 size = 0;

    if (!mem_size) {
        nr_bank = get_dram_rank_nr();

        get_dram_rank_size(rank_size);

        for (i = 0; i < nr_bank; i++)
            size += (u32)*rksize++;
        mem_size = size;
    }

    return mem_size;
}

void platform_pre_init(void)
{
    u32 pmic_ret;
    u32 pwrap_ret=0,i=0;
    u32 ret = 0;
    struct rtc_time tm;

#ifdef MTK_TEST_TMODE
    trigger_z93_test_even(MTK_TEST_TMODE);
#endif

    /* init timer */
    mtk_timer_init();

    /* init boot time */
    g_boot_time = get_timer(0);

#if !CFG_FPGA_PLATFORM
    /* init pll */
    mt_pll_init();
    BOOTING_TIME_PROFILING_LOG("PLL");
#endif

    /* init uart baudrate when pll on */
    mtk_uart_init(UART_SRC_CLK_FRQ, CFG_LOG_BAUDRATE);

    /*GPIO init*/
    mt_gpio_init();
    BOOTING_TIME_PROFILING_LOG("GPIO");

#if !CFG_FPGA_PLATFORM

    #if (CFG_USB_UART_SWITCH)
	if (is_uart_cable_inserted()) {
		print("\n%s Switch to UART Mode\n", MOD);
		set_to_uart_mode();
	} else {
		print("\n%s Keep stay in USB Mode\n", MOD);
	}
    BOOTING_TIME_PROFILING_LOG("USB SWITCH to UART");
    #endif
#endif

//#if !CFG_FPGA_PLATFORM
    /* init pll post */
//    mt_pll_post_init();
//	BOOTING_TIME_PROFILING_LOG("PLL post");
//#endif

    //retry 3 times for pmic wrapper init
    pwrap_init_preloader();
    BOOTING_TIME_PROFILING_LOG("PWRAP");

    //print rtc time
    rtc_get_time(&tm);
    print("Current RTC time:[%d/%d/%d %d:%d:%d]\n", tm.tm_year, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    //i2c hw init
    i2c_hw_init();

    //enable_devinfo_dcm
    enable_devinfo_dcm();

    //show ic debug information
    show_devinfo_debug_info();

    BOOTING_TIME_PROFILING_LOG("I2C");
#if !CFG_FPGA_PLATFORM
    pmic_ret = pmic_init();
    BOOTING_TIME_PROFILING_LOG("PMIC");

#if CFG_CPU_FAST_FREQ
	/* adjust vproc/vsram */
	buck_adjust_voltage(VSRAM_PROC, PMIC_VOLT_01_200000_V);
	buck_adjust_voltage(VPROC, PMIC_VOLT_01_000000_V);
#else
	print("vproc/vsram run as hw default\n");
#endif

#if CFG_DRAM_CALIB_OPTIMIZATION || CFG_PRELOADER_EXTENSION
	/*
	 * Init booting storage device.
	 *
	 * If CFG_DRAM_CALIB_OPTIMIZATION is enabled, booting storage device
	 * can be initialized before DRAM initialization.
	 */
	ret = boot_device_init();
	BOOTING_TIME_PROFILING_LOG("Boot dev init");
	print("%s Init Boot Device: %s(%d)\n", MOD, ret ? "FAIL" : "OK", ret);
#endif

    /* init security library */
    sec_lib_init();
    BOOTING_TIME_PROFILING_LOG("Sec lib init");
    /* note: lock state is only valid after sec_lib_init, be careful when moving the following functions */

#if defined(MTK_AB_OTA_UPDATER)
    ab_ota_boot_check();

    /* This is for MT6739 only.
     * Since preloader needs to load loader_ext_dram, the time of executing
     * ab_ota_boot_check() is earlier than other platform. Once g_boot_mode
     * is assigned as RECOVERY_BOOT in ab_ota_boot_check(), it would be
     * overwritten as NORMAL_BOOT in bldr_pre_process(). Therefore we back up
     * g_boot_mode and resume it later if it is RECOVERY_BOOT.
     */
    ab_boot_mode = g_boot_mode;
#endif

#if CFG_PRELOADER_EXTENSION && CFG_DRAM_CALIB_OPTIMIZATION
    bldr_load_loader_ext_dram();
    BOOTING_TIME_PROFILING_LOG("loader_ext_dram");
#endif

    mt_pll_post_init();

    //mt_arm_pll_sel();
    BOOTING_TIME_PROFILING_LOG("PLL POST Init");
#endif

    //enable long press reboot function
    PMIC_enable_long_press_reboot();
    BOOTING_TIME_PROFILING_LOG("Long Pressed Reboot");

    print("%s Init PWRAP: %s(%d)\n", MOD, pwrap_ret ? "FAIL" : "OK", pwrap_ret);
    print("%s Init PMIC: %s(%d)\n", MOD, pmic_ret ? "FAIL" : "OK", pmic_ret);

    platform_core_handler();

    print("%s chip_ver[%x]\n", MOD, platform_chip_ver());
#if defined(MTK_PTP)
    if (!pmic_ret)
    	ptp_init();
#endif //defined(MTK_PTP)
}


#ifdef MTK_MT8193_SUPPORT
extern int mt8193_init(void);
#endif


void platform_init(void)
{
    u32 ret, tmp;
    boot_reason_t reason;
    blkdev_t *bootdev = NULL;

#if !CFG_FPGA_PLATFORM
    if (clk_buf_init())
	    print("%s: clk_buf_init fail\n", __func__);
    rtc_init();

    pmic_init_setting();
#endif

#if !CFG_BYPASS_EMI
    /* check DDR-reserve mode */
    check_ddr_reserve_status();
    BOOTING_TIME_PROFILING_LOG("chk DDR Reserve status");
#endif

    /* init watch dog, will enable AP watch dog */
    mtk_wdt_init();
    /*init kpd PMIC mode support*/
    set_kpd_pmic_mode();


#if CFG_MDWDT_DISABLE
    /* no need to disable MD WDT, the code here is for backup reason */

    /* disable MD0 watch dog. */
    DRV_WriteReg32(0x20050000, 0x2200);

    /* disable MD1 watch dog. */
    DRV_WriteReg32(0x30050020, 0x2200);
#endif


#if !CFG_FPGA_PLATFORM/* FIXME */
    g_boot_reason = reason = platform_boot_status();
    BOOTING_TIME_PROFILING_LOG("check Boot status");

    if (reason == BR_RTC || reason == BR_POWER_KEY || reason == BR_USB || reason == BR_WDT || reason == BR_WDT_BY_PASS_PWK || reason == BR_2SEC_REBOOT)
        rtc_bbpu_power_on();
#else

    g_boot_reason = BR_POWER_KEY;

#endif
    BOOTING_TIME_PROFILING_LOG("rtc_bbpu_power_on");

#if !CFG_FPGA_PLATFORM
		pl_battery_init(false);
#endif

    enable_PMIC_kpd_clock();
    BOOTING_TIME_PROFILING_LOG("Enable PMIC Kpd clk");

#if CFG_UART_DYNAMIC_SWITCH
#ifdef KPD_DL_KEY1
    mtk_kpd_gpio_set();
    if (mtk_detect_key(KPD_DL_KEY1)) {
        print("Vol Up detected. Log Keep on.\n");
    } else {
        print("Log Turned Off.\n");
        set_log_switch(0);
    }
    BOOTING_TIME_PROFILING_LOG("UART DYNAMIC SWITCH");
#else
    print("KPD_DL_KEY1 not define\n");
#endif
#endif //CFG_UART_DYNAMIC_SWITCH

#ifdef MTK_TRNG_CALIBRATION
#if defined MTK_TRNG_CALIBRATION_1ST_ROUND
	ret = trng_calib_1st_round(g_trng_buf, TRNG_CALIB_BUF_SIZE);
	if (ret) {
		print("trng calibration failed\n");
	}
	/* trng calibration terminates here */
	while(1);
#endif //MTK_TRNG_CALIBRATION_1ST_ROUND
#endif //MTK_TRNG_CALIBRATION

    /* init memory */
#if !CFG_BYPASS_EMI
#if !CFG_FPGA_PLATFORM
    mt_mem_init();
    BOOTING_TIME_PROFILING_LOG("mem_init");
#endif
#endif

    /*init dram buffer*/
    init_dram_buffer();
    BOOTING_TIME_PROFILING_LOG("Init Dram buf");

    /* switch log buffer to dram */
    //log_buf_ctrl(1);
#if 0 /* FIXME */
    /* enable CA9 share bits for USB(30)/NFI(29)/MSDC(28) modules to access ISRAM */
    tmp = DRV_Reg32(0xC1000200);
    tmp |= ((1<<30)|(1<<29)|(1<<28));
    DRV_WriteReg32 (0xC1000200, tmp);
#endif

#ifdef MTK_MT8193_SUPPORT
	mt8193_init();
#endif

#if !CFG_BYPASS_EMI
	ram_console_init();
	ram_console_reboot_reason_save(g_rgu_status);
	BOOTING_TIME_PROFILING_LOG("Ram console");
#endif

#if !CFG_DRAM_CALIB_OPTIMIZATION
	/*
	 * Init booting storage device.
	 *
	 * If CFG_DRAM_CALIB_OPTIMIZATION is disabled or not defined, booting storage device
	 * needs to be initialized after DRAM initialization.
	 */
    ret = boot_device_init();
    BOOTING_TIME_PROFILING_LOG("Boot dev init");
    print("%s Init Boot Device: %s(%d)\n", MOD, ret ? "FAIL" : "OK", ret);
#endif

#if CFG_EMERGENCY_DL_SUPPORT
    /* check if to enter emergency download mode */
    /* Move after dram_inital and boot_device_init.
      Use excetution time to remove delay time in mtk_kpd_gpio_set()*/
    if (mtk_detect_dl_keys()) {
        platform_emergency_download(CFG_EMERGENCY_DL_TIMEOUT_MS);
    }
#endif

#ifdef MTK_TRNG_CALIBRATION
#if defined MTK_TRNG_CALIBRATION_2ND_ROUND
	ret = trng_calib_2nd_round(g_trng_buf, TRNG_CALIB_BUF_SIZE);
	if (ret) {
		print("trng calibration failed\n");
	}
	/* trng calibration terminates here */
	while(1);
#endif //MTK_TRNG_CALIBRATION_2ND_ROUND
#endif //MTK_TRNG_CALIBRATION
#if CFG_REBOOT_TEST
    mtk_wdt_sw_reset();
    while(1);
#endif

	/* make sure usb11 in power saving mode*/
	mt_usb11_phy_savecurrent();

#if !CFG_FPGA_PLATFORM
/*In FPGA phase, dram related function should by pass*/
    bootarg.dram_rank_num = get_dram_rank_nr();
    get_dram_rank_size(bootarg.dram_rank_size);
    get_orig_dram_rank_info(&bootarg.orig_dram_info);
    setup_mblock_info(&bootarg.mblock_info, &bootarg.orig_dram_info, &bootarg.lca_reserved_mem);
#else
    /*pass a defaut dram info to LK*/ //512 + 512MB
    bootarg.dram_rank_num = 0x00000002;
    bootarg.dram_rank_size[0] = 0x20000000;
    bootarg.dram_rank_size[1] = 0x20000000;
    bootarg.dram_rank_size[2] = 0xdfffffff;
    bootarg.dram_rank_size[3] = 0xfffffdff;
    bootarg.mblock_info.mblock_num = 0x00000002;
    bootarg.mblock_info.mblock[0].start = 0x40000000;
    bootarg.mblock_info.mblock[0].size = 0x20000000;
    bootarg.mblock_info.mblock[0].rank = 0;
    bootarg.mblock_info.mblock[1].start = 0x60000000;
    bootarg.mblock_info.mblock[1].size = 0x1fe00000;
    bootarg.mblock_info.mblock[1].rank = 1;
    bootarg.mblock_info.mblock[2].start = 0;
    bootarg.mblock_info.mblock[2].size = 0;
    bootarg.mblock_info.mblock[2].rank = 0;
    bootarg.mblock_info.mblock[3].start = 0;
    bootarg.mblock_info.mblock[3].size = 0;
    bootarg.mblock_info.mblock[3].rank = 0;
    bootarg.orig_dram_info.rank_num = 2;
    bootarg.orig_dram_info.rank_info[0].start = 0x40000000;
    bootarg.orig_dram_info.rank_info[0].size = 0x20000000;
    bootarg.orig_dram_info.rank_info[1].start = 0x60000000;
    bootarg.orig_dram_info.rank_info[1].size = 0x20000000;
    bootarg.orig_dram_info.rank_info[2].start = 0;
    bootarg.orig_dram_info.rank_info[2].size = 0;
    bootarg.orig_dram_info.rank_info[3].start = 0;
    bootarg.orig_dram_info.rank_info[3].size = 0;
    bootarg.lca_reserved_mem.start = 0;
    bootarg.lca_reserved_mem.size = 0;
#endif
    emi_reserve_4k_workaround();
}

void platform_post_init(void)
{
#ifdef MTK_SECURITY_SW_SUPPORT
	/*Anti rollback update*/
#ifdef MTK_SECURITY_ANTI_ROLLBACK
	u32 g_pl_otp_status = 0x0;

	g_pl_otp_status = sec_otp_ver_update();
	if (g_pl_otp_status)
		print("update fail 0x%x\n", g_pl_otp_status);
#endif
#endif

#if CFG_FUNCTION_PICACHU_SUPPORT
    start_picachu();
    etc_entry();
#endif
#if CFG_BATTERY_DETECT
    /* normal boot to check battery exists or not */
    if (g_boot_mode == NORMAL_BOOT && !hw_check_battery() && usb_accessory_in()) {
        print("%s Wait for battery inserted...\n", MOD);
        /* disable pmic pre-charging led */
        //remove empty function to save space pl_close_pre_chr_led();
        /* enable force charging mode */
        //remove empty function to save space pl_charging(1);
        do {
            mdelay(300);
            /* check battery exists or not */
            if (hw_check_battery())
                break;
            /* kick all watchdogs */
            platform_wdt_all_kick();
        } while(1);
        /* disable force charging mode */
        //remove empty function to save space pl_charging(0);
    }
#endif

#if !CFG_FPGA_PLATFORM
		pl_battery_init(true);
#endif
    BOOTING_TIME_PROFILING_LOG("Battery detect");


#if CFG_MDJTAG_SWITCH
    unsigned int md_pwr_con;

    /* md0 default power on and clock on */
    /* md1 default power on and clock off */

    /* ungate md1 */
    /* rst_b = 0 */
    md_pwr_con = DRV_Reg32(0x10006280);
    md_pwr_con &= ~0x1;
    DRV_WriteReg32(0x10006280, md_pwr_con);

    /* enable clksq2 for md1 */
    DRV_WriteReg32(0x10209000, 0x00001137);
    udelay(200);
    DRV_WriteReg32(0x10209000, 0x0000113f);

    /* rst_b = 1 */
    md_pwr_con = DRV_Reg32(0x10006280);
    md_pwr_con |= 0x1;
    DRV_WriteReg32(0x10006280, md_pwr_con);

    /* switch to MD legacy JTAG */
    /* this step is not essentially required */
#endif
    BOOTING_TIME_PROFILING_LOG("MTJTAG switch");

#if CFG_MDMETA_DETECT
    if (g_boot_mode == META_BOOT || g_boot_mode == ADVMETA_BOOT) {
	/* trigger md0 to enter meta mode */
        DRV_WriteReg32(0x20000010, 0x1);
	/* trigger md1 to enter meta mode */
        DRV_WriteReg32(0x30000010, 0x1);
    } else {
	/* md0 does not enter meta mode */
        DRV_WriteReg32(0x20000010, 0x0);
	/* md1 does not enter meta mode */
        DRV_WriteReg32(0x30000010, 0x0);
    }
#endif
    BOOTING_TIME_PROFILING_LOG("MTMETA Detect");

    platform_set_boot_args();
    BOOTING_TIME_PROFILING_LOG("Boot Argu");
}

void platform_error_handler(void)
{
    int i = 0;
    /* if log is disabled, re-init log port and enable it */
    if (log_status() == 0) {
        mtk_uart_init(UART_SRC_CLK_FRQ, CFG_LOG_BAUDRATE);
        log_ctrl(1);
    }
    print("PL fatal error...\n");

    #if !CFG_FPGA_PLATFORM
    sec_util_brom_download_recovery_check();
    #endif

#if defined(ONEKEY_REBOOT_NORMAL_MODE_PL) || defined(TWOKEY_REBOOT_NORMAL_MODE_PL)
    /* add delay for Long Preessed Reboot count down
       only pressed power key will have this delay */
    print("PL delay for Long Press Reboot\n");
    for ( i=3; i > 0;i-- ) {
        if (mtk_detect_key(PL_PMIC_PWR_KEY)) {
            platform_wdt_kick();
            mdelay(5000);   //delay 5s/per kick,
        } else {
            break; //Power Key Release,
        }
    }
#endif

    /* enter emergency download mode */
    #if CFG_EMERGENCY_DL_SUPPORT
    platform_emergency_download(CFG_EMERGENCY_DL_TIMEOUT_MS);
    #endif

    while(1);
}

void platform_assert(char *file, int line, char *expr)
{
    print("<ASSERT> %s:line %d %s\n", file, line, expr);
    platform_error_handler();
}

int aarch64_slt_done(void)
{
#if CFG_LOAD_SLT_AARCH64_KERNEL
    if ((*(unsigned int*) AARCH64_SLT_DONE_ADDRESS) == AARCH64_SLT_DONE_MAGIC)
    {
        return 1;
    }
    else
    {
        return 0;
    }
#endif
    return 1;
}


