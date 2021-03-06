LOCAL_DIR := $(GET_LOCAL_DIR)

ARCH    := arm
ARM_CPU := cortex-a9
CPU     := generic


MMC_SLOT         := 1

DEFINES += PERIPH_BLK_BLSP=1 
DEFINES += WITH_CPU_EARLY_INIT=0 WITH_CPU_WARM_BOOT=0 \
	   MMC_SLOT=$(MMC_SLOT)

ifeq ($(MTK_SECURITY_SW_SUPPORT), yes)
	DEFINES += MTK_SECURITY_SW_SUPPORT
endif	   
$(info libshowlogo new path ------- $(LOCAL_DIR)/../../../../bootable/bootloader/lk/lib/libshowlogo)
INCLUDES += -I$(LOCAL_DIR)/include \
            -I$(LOCAL_DIR)/../../../../bootable/bootloader/lk/lib/libshowlogo \
	    -Iout/lk/lcm/inc \
	    -Iout/lk/inc \
	    -Iout/common \
	    -Iout/kernel/dct/ 
OBJS += \
	$(LOCAL_DIR)/bitops.o \
	$(LOCAL_DIR)/platform.o \
	$(LOCAL_DIR)/pwm.o \
	$(LOCAL_DIR)/uart.o \
	$(LOCAL_DIR)/interrupts.o \
	$(LOCAL_DIR)/timer.o \
	$(LOCAL_DIR)/debug.o \
	$(LOCAL_DIR)/i2c.o \
	$(LOCAL_DIR)/boot_mode.o \
	$(LOCAL_DIR)/load_image.o \
	$(LOCAL_DIR)/atags.o \
	$(LOCAL_DIR)/mt_partition.o \
	$(LOCAL_DIR)/addr_trans.o \
	$(LOCAL_DIR)/mmc_core.o \
	$(LOCAL_DIR)/mmc_test.o \
	$(LOCAL_DIR)/msdc_utils.o \
	$(LOCAL_DIR)/msdc.o \
	$(LOCAL_DIR)/pll.o \
	$(LOCAL_DIR)/factory.o \
	$(LOCAL_DIR)/mtk_gpt.o\
	$(LOCAL_DIR)/mtk_sleep.o\
	$(LOCAL_DIR)/mt_rtc.o\
	$(LOCAL_DIR)/mt_usb.o\
	$(LOCAL_DIR)/mtk_key.o \
	$(LOCAL_DIR)/mt_disp_drv.o\
	$(LOCAL_DIR)/disp_drv.o\
	$(LOCAL_DIR)/disp_assert_layer.o\
	$(LOCAL_DIR)/disp_drv_dbi.o\
	$(LOCAL_DIR)/disp_drv_dpi.o\
	$(LOCAL_DIR)/disp_drv_dsi.o\
	$(LOCAL_DIR)/lcd_drv.o\
	$(LOCAL_DIR)/dpi_drv.o\
	$(LOCAL_DIR)/dsi_drv.o\
	$(LOCAL_DIR)/partition_mt.o\
        $(LOCAL_DIR)/mt_get_dl_info.o \
	$(LOCAL_DIR)/mtk_wdt.o\
	$(LOCAL_DIR)/mt_leds.o\
	$(LOCAL_DIR)/recovery.o\
	$(LOCAL_DIR)/meta.o\
	$(LOCAL_DIR)/mt_logo.o\
	$(LOCAL_DIR)/mt_gpio.o\
	$(LOCAL_DIR)/mt_gpio_init.o\
	$(LOCAL_DIR)/mtk_nand.o\
	$(LOCAL_DIR)/bmt.o\
	$(LOCAL_DIR)/boot_mode_menu.o\
	$(LOCAL_DIR)/env.o\
	$(LOCAL_DIR)/bat_id.o\

ifeq ($(MTK_BQ24158_SUPPORT),yes)
	OBJS += $(LOCAL_DIR)/bq24158.o
	OBJS += $(LOCAL_DIR)/mt_bat_bq24158.o
else
ifeq ($(MTK_FAN5405_SUPPORT),yes)
	OBJS += $(LOCAL_DIR)/fan5405.o
	OBJS += $(LOCAL_DIR)/mt_bat_fan5405.o
else
    ifeq ($(MTK_NCP1851_SUPPORT),yes)
        OBJS += $(LOCAL_DIR)/ncp1851.o
        OBJS += $(LOCAL_DIR)/mt_bat_tbl.o        
    else
	      OBJS += $(LOCAL_DIR)/mt_battery.o
    endif
endif
endif

ifeq ($(CUSTOM_KERNEL_TOUCHPANEL),GT827)
    OBJS += $(LOCAL_DIR)/mt_pmic_tbl.o
else
    OBJS += $(LOCAL_DIR)/mt_pmic.o
endif

ifeq ($(CUSTOM_SEC_AUTH_SUPPORT), yes)
LIBSEC := -L$(LOCAL_DIR)/lib -lsec
else
LIBSEC := -L$(LOCAL_DIR)/lib -lsec -lauth
endif
LIBSEC_PLAT := -lsplat -ldevinfo

LINKER_SCRIPT += $(BUILDDIR)/system-onesegment.ld
