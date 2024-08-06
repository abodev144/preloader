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

#include "typedefs.h"
#include "platform.h"
#include "bootctrl.h"
#include "ab_bootcheck.h"
#include "pal_log.h"

#define MOD "[AB]"

char *ab_suffix = NULL;

static int ab_get_boot_part(u32 *bootpart)
{
    int ret = 0;
    u32 boot_part = 0;
#if (CFG_BOOT_DEV == BOOTDEV_SDMMC)
    ret = mmc_get_boot_part(&boot_part);

    if(boot_part == EMMC_PART_BOOT1)
        *bootpart = BOOT_PART_A;
    else if(boot_part == EMMC_PART_BOOT2)
        *bootpart = BOOT_PART_B;
#elif (CFG_BOOT_DEV == BOOTDEV_UFS)
    extern struct ufs_hba g_ufs_hba;
    struct ufs_hba *hba = &g_ufs_hba;

    ret = ufs_aio_get_boot_lu(hba, &boot_part);
    if(boot_part == ATTR_B_BOOT_LUN_EN_BOOT_LU_A)
        *bootpart = BOOT_PART_A;
    else if(boot_part == ATTR_B_BOOT_LUN_EN_BOOT_LU_B)
        *bootpart = BOOT_PART_B;
#endif
    return ret;
}

static int ab_set_boot_part(u32 bootpart)
{
#if (CFG_BOOT_DEV == BOOTDEV_SDMMC)
    return mmc_set_boot_part(bootpart);
#elif (CFG_BOOT_DEV == BOOTDEV_UFS)
    extern struct ufs_hba g_ufs_hba;
    struct ufs_hba *hba = &g_ufs_hba;

    return ufs_aio_set_boot_lu(hba, bootpart);
#endif
}

void ab_ota_boot_check(void)
{
    ab_suffix = get_suffix();
    u8 ab_retry = get_retry_count(ab_suffix);
    boot_part_t bootpart = BOOT_PART_NONE;

    if (ab_get_boot_part(&bootpart)) {
        pal_log_info("%s invalid boot part: %d\n", MOD, bootpart);
    }
    if(bootpart == BOOT_PART_A)
        pal_log_info("%s Current boot: Preloader_a\n", MOD);
    else if(bootpart == BOOT_PART_B)
        pal_log_info("%s Current boot: Preloader_b\n", MOD);
    else
        pal_log_info("%s Current boot: %d\n", MOD, bootpart);

    while (!check_valid_slot()) { //one or more valid slot
        pal_log_info("%s ab_suffix: %s, ab_retry: %d\n", MOD, ab_suffix, ab_retry);
        if (get_boot_mode(ab_suffix)) { //boot to normal mode
            if (get_bootup_status(ab_suffix)) { //boot_successfully == 1
                return;
            } else {
                if (ab_retry > 0) {
                    reduce_retry_count(ab_suffix);
                    return;
                } else {
                    mark_slot_invalid(ab_suffix);
                    if (check_valid_slot() == -1)
                        break;
                    //rollback to previous suffix
                    pal_log_info("%s rollback suffix!\n", MOD);
                    if (!memcmp(ab_suffix, BOOTCTRL_SUFFIX_A, 2)) {
                        ab_suffix = BOOTCTRL_SUFFIX_B;

                        if (ab_set_boot_part(BOOT_PART_B)) {
                            pal_log_err("Fail switch to Preloader_b\n");
                            ASSERT(0);
                        }
                        else { //success switch preloader to previous version, reboot
                            pal_log_err("Success switch to Preloader_b\n");
                            mtk_arch_reset(1);
                        }
                    } else {
                        ab_suffix = BOOTCTRL_SUFFIX_A;

                        if (ab_set_boot_part(BOOT_PART_A)) {
                            pal_log_err("Fail switch to Preloader_a\n");
                            ASSERT(0);
                        }
                        else { //success switch preloader to previous version, reboot
                            pal_log_err("Success switch to Preloader_a\n");
                            mtk_arch_reset(1);
                        }
                    }
                    ab_retry = get_retry_count(ab_suffix);
                }
            }
        } else {
            return;
        }
    }
    g_boot_mode = RECOVERY_BOOT;
    pal_log_err("%s no valid slot! boot_mode: %d\n", MOD, g_boot_mode);
    return;
}
