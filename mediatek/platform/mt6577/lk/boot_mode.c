/*
 * Copyright (c) 2008-2009 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/*This file implements MTK boot mode.*/

#include <sys/types.h>
#include <debug.h>
#include <err.h>
#include <reg.h>

#include <platform/mt_typedefs.h>
#include <platform/boot_mode.h>
#include <platform/mt_reg_base.h>
#include <platform/meta.h>
#include <platform/mt_rtc.h>
#include <platform/mtk_key.h>
#include <platform/mt_gpt.h>
#include <platform/mtk_wdt.h>
#include <target/cust_key.h>


// global variable for specifying boot mode (default = NORMAL)
extern  int unshield_recovery_detection(void);
extern  void mtk_wdt_disable(void);
extern  void boot_mode_menu_select();
BOOTMODE g_boot_mode = NORMAL_BOOT;

BOOL meta_mode_check(void)
{
	if(g_boot_mode == META_BOOT || g_boot_mode == ADVMETA_BOOT || g_boot_mode ==ATE_FACTORY_BOOT)
	{
	  return TRUE;
	}
	else
	{	return FALSE;
	}
}

extern BOOT_ARGUMENT *g_boot_arg;
#define MODULE_NAME "[FACTORY]"

// check the boot mode : (1) meta mode or (2) recovery mode ...
void boot_mode_select(void)
{
    int factory_forbidden = 0;

    /*We put conditions here to filer some cases that can not do key detection*/
    if (meta_detection())
    {
      return;
    }

#if defined (HAVE_LK_TEXT_MENU)
    /*Check RTC to know if system want to reboot to Fastboot*/
    if(Check_RTC_PDN1_bit13())
    {
      printf("[FASTBOOT] reboot to boot loader\n");
      g_boot_mode = FASTBOOT;
      Set_Clr_RTC_PDN1_bit13(false);
   	  return;
    }

    /*If forbidden mode is factory, cacel the factory key detection*/ 
    if(g_boot_arg->sec_limit.magic_num == 0x4C4C4C4C)
    {
        if(g_boot_arg->sec_limit.forbid_mode == F_FACTORY_MODE)
        {
            //Forbid to enter factory mode
            printf("%s Forbidden\n",MODULE_NAME);
            factory_forbidden=1;
        }
    }
    /*If boot reason is power key + volumn down, then
     disable factory mode dectection*/
    if(mtk_detect_pmic_just_rst())
    {
          factory_forbidden=1;
    }

    /*Check RTC to know if system want to reboot to Recovery*/
    if ((DRV_Reg16(RTC_PDN1) & 0x0030) == 0x0010)
    {
        g_boot_mode = RECOVERY_BOOT;
      return;
    }

    /*If MISC Write has not completed  in recovery mode 
      before system reboot, go to recovery mode to
      finish remain tasks*/     
    if(unshield_recovery_detection())
    {
        return;
    }

    /*we put key dectection here to detect key which is pressed*/
    printf("eng build\n");
    printf("MT65XX_FACTORY_KEY 0x%x\n",MT65XX_FACTORY_KEY);
    printf("MT65XX_BOOT_MENU_KEY 0x%x\n",MT65XX_BOOT_MENU_KEY);
    printf("MT65XX_RECOVERY_KEY 0x%x\n",MT65XX_RECOVERY_KEY);
    ulong begin = get_timer(0);
    while(get_timer(begin)<50)
    {
        if(!factory_forbidden)
        {
            if(mtk_detect_key(MT65XX_FACTORY_KEY))
  		{
                printf("%s Detect key\n",MODULE_NAME);
                printf("%s Enable factory mode\n",MODULE_NAME);
                g_boot_mode = FACTORY_BOOT;
                //video_printf("%s : detect factory mode !\n",MODULE_NAME);
                return;
            }
  		}

        if(mtk_detect_key(MT65XX_BOOT_MENU_KEY))
  		{
            printf("\n%s Check  boot menu\n",MODULE_NAME);
            printf("%s Wait 50ms for special keys\n",MODULE_NAME);
            mtk_wdt_disable();
            boot_mode_menu_select();
            mtk_wdt_init();
            return;  
  		}

#ifdef MT65XX_RECOVERY_KEY
        if(mtk_detect_key(MT65XX_RECOVERY_KEY))
        {
            printf("%s Detect cal key\n",MODULE_NAME);
            printf("%s Enable recovery mode\n",MODULE_NAME);
            g_boot_mode = RECOVERY_BOOT;
            //video_printf("%s : detect recovery mode !\n",MODULE_NAME);
    	return;
    }
#endif                   
    }
#else
#ifdef MTK_FASTBOOT_SUPPORT
    /*Check RTC to know if system want to reboot to Fastboot*/
    if(Check_RTC_PDN1_bit13())
    {
        dprintf(INFO,"[FASTBOOT] reboot to boot loader\n");
        g_boot_mode = FASTBOOT;
        Set_Clr_RTC_PDN1_bit13(false);
        return;
    }
#endif

    /*If forbidden mode is factory, cacel the factory key detection*/
    if(g_boot_arg->sec_limit.magic_num == 0x4C4C4C4C)
    {
        if(g_boot_arg->sec_limit.forbid_mode == F_FACTORY_MODE)
        {
            //Forbid to enter factory mode
            printf("%s Forbidden\n",MODULE_NAME);
            factory_forbidden=1;
        }
    }
    /*If boot reason is power key + volumn down, then 
     disable factory mode dectection*/
    if(mtk_detect_pmic_just_rst())
    {
          factory_forbidden=1;
    }

    /*Check RTC to know if system want to reboot to Recovery*/
    if ((DRV_Reg16(RTC_PDN1) & 0x0030) == 0x0010)
  		{
        g_boot_mode = RECOVERY_BOOT;
        return;
  		}

    /*If MISC Write has not completed  in recovery mode 
      and interrupted by system reboot, go to recovery mode to 
      finish remain tasks*/
    if(unshield_recovery_detection())
  		{
        return;
  		}

    /*we put key dectection here to detect key which is pressed*/
    ulong begin = get_timer(0);
    while(get_timer(begin)<50){
#ifdef MTK_FASTBOOT_SUPPORT
        if(mtk_detect_key(MT_CAMERA_KEY))
        {
            dprintf(INFO,"[FASTBOOT]Key Detect\n");
            g_boot_mode = FASTBOOT;
            return;
    }
#endif

        if(!factory_forbidden)
        {
            if(mtk_detect_key(MT65XX_FACTORY_KEY))
  		    {
                printf("%s Detect key\n",MODULE_NAME);
                printf("%s Enable factory mode\n",MODULE_NAME);
                g_boot_mode = FACTORY_BOOT;
                //video_printf("%s : detect factory mode !\n",MODULE_NAME);
                return;
  		    }
}

#ifdef MT65XX_RECOVERY_KEY
        if(mtk_detect_key(MT65XX_RECOVERY_KEY))
{
            printf("%s Detect cal key\n",MODULE_NAME);
            printf("%s Enable recovery mode\n",MODULE_NAME);
            g_boot_mode = RECOVERY_BOOT;
            //video_printf("%s : detect recovery mode !\n",MODULE_NAME);
            return;
  		}
#endif
}
#endif
}

CHIP_VER get_chip_eco_ver(void)
{
    return DRV_Reg32(APHW_VER);
}

CHIP_VER get_chip_ver(void)
{
    unsigned int hw_subcode = DRV_Reg32(APHW_SUBCODE);
    unsigned int sw_ver = DRV_Reg32(APSW_VER);
    CHIP_VER ver = CHIP_6575_E2;

    if (0x8A00 == hw_subcode) {
        if (0xE100 == sw_ver) {
            ver = CHIP_6575_E1;
        } else if (0xE201 == sw_ver) {
            if (0x40000000 == (DRV_Reg32(0xC1019100) & 0x40000000)) {
                ver = CHIP_6575_E3;
            } else {
                ver = CHIP_6575_E2;
            }
        }
    } else if (0x8B00 == hw_subcode) {
        ver = CHIP_6577_E1;
    }
    return ver;
}
