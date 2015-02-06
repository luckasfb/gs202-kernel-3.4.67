###################################################################
# Include Files Directery
###################################################################

#include $(D_ROOT)/mtk_cust.mak

###################################################################
# Using GCC
###################################################################

CROSS_COMPILE = arm-linux-androideabi-

AS	= $(CROSS_COMPILE)as
LD	= $(CROSS_COMPILE)ld
CC	= $(CROSS_COMPILE)gcc
CPP	= $(CC)-E
AR	= $(CROSS_COMPILE)ar
NM	= $(CROSS_COMPILE)nm
STRIP	= $(CROSS_COMPILE)strip
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
RANLIB	= $(CROSS_COMPILE)RANLIB
THUMB_MODE = TRUE

###################################################################
# Initialize GCC Compile Parameter
###################################################################
#//<20121009-14993-Eric Lin, [Hawk4.0][secu] Include OpenSSL related files into make file.
ifeq ($(MTK_EMMC_SUPPORT), yes)
#DEFINE          	:= -D$(MTK_PLATFORM) \
#		   		-DMTK_EMMC_SUPPORT 

ifeq ($(MTK_SEC_MP_KEY), KEY_TYPE_MTK)
DEFINE           := -D$(MTK_PLATFORM) \
			     -DMTK_EMMC_SUPPORT \
			     -DKEY_TYPE_MTK \
				-D__SECURE_USB_DOWNLOAD__\
				-D__MOTO_USB_DOWNLOAD__  \
				-DOPENSSL_NO_SOCK        \
				-DOPENSSL_NO_BIO         \
				-DOPENSSL_NO_LHASH       \
				-DOPENSSL_NO_ERR         \
				-DOPENSSL_NO_CMS         \
				-DOPENSSL_NO_ENGINE      \
				-DOPENSSL_NO_EC          \
				-DOPENSSL_NO_ECDSA       \
				-DOPENSSL_NO_ECDH        \
				-DOPENSSL_NO_DSA         \
				-DOPENSSL_NO_DH          \
				-DOPENSSL_NO_LOCKING  \
				-DOPENSSL_SMALL_FOOTPRINT
endif
ifeq ($(MTK_SEC_MP_KEY), KEY_TYPE_DEV)
DEFINE           := -D$(MTK_PLATFORM) \
			     -DMTK_EMMC_SUPPORT \
			     -DKEY_TYPE_DEV \
				-D__SECURE_USB_DOWNLOAD__\
				-D__MOTO_USB_DOWNLOAD__  \
				-DOPENSSL_NO_SOCK        \
				-DOPENSSL_NO_BIO         \
				-DOPENSSL_NO_LHASH       \
				-DOPENSSL_NO_ERR         \
				-DOPENSSL_NO_CMS         \
				-DOPENSSL_NO_ENGINE      \
				-DOPENSSL_NO_EC          \
				-DOPENSSL_NO_ECDSA       \
				-DOPENSSL_NO_ECDH        \
				-DOPENSSL_NO_DSA         \
				-DOPENSSL_NO_DH          \
				-DOPENSSL_NO_LOCKING  \
				-DOPENSSL_SMALL_FOOTPRINT			     
endif
ifeq ($(MTK_SEC_MP_KEY), KEY_TYPE_MP)
DEFINE           := -D$(MTK_PLATFORM) \
			     -DMTK_EMMC_SUPPORT \
			     -DKEY_TYPE_MP \
				-D__SECURE_USB_DOWNLOAD__\
				-D__MOTO_USB_DOWNLOAD__  \
				-DOPENSSL_NO_SOCK        \
				-DOPENSSL_NO_BIO         \
				-DOPENSSL_NO_LHASH       \
				-DOPENSSL_NO_ERR         \
				-DOPENSSL_NO_CMS         \
				-DOPENSSL_NO_ENGINE      \
				-DOPENSSL_NO_EC          \
				-DOPENSSL_NO_ECDSA       \
				-DOPENSSL_NO_ECDH        \
				-DOPENSSL_NO_DSA         \
				-DOPENSSL_NO_DH          \
				-DOPENSSL_NO_LOCKING  \
				-DOPENSSL_SMALL_FOOTPRINT			     
endif

else
#DEFINE          	:= -D$(MTK_PLATFORM)

ifeq ($(MTK_SEC_MP_KEY), KEY_TYPE_MTK)
	DEFINE  := -D$(MTK_PLATFORM) -DKEY_TYPE_MTK \
				-D__SECURE_USB_DOWNLOAD__\
				-D__MOTO_USB_DOWNLOAD__  \
				-DOPENSSL_NO_SOCK        \
				-DOPENSSL_NO_BIO         \
				-DOPENSSL_NO_LHASH       \
				-DOPENSSL_NO_ERR         \
				-DOPENSSL_NO_CMS         \
				-DOPENSSL_NO_ENGINE      \
				-DOPENSSL_NO_EC          \
				-DOPENSSL_NO_ECDSA       \
				-DOPENSSL_NO_ECDH        \
				-DOPENSSL_NO_DSA         \
				-DOPENSSL_NO_DH          \
				-DOPENSSL_NO_LOCKING  \
				-DOPENSSL_SMALL_FOOTPRINT	
endif
ifeq ($(MTK_SEC_MP_KEY), KEY_TYPE_DEV)
	DEFINE  := -D$(MTK_PLATFORM) -DKEY_TYPE_DEV\
				-D__SECURE_USB_DOWNLOAD__\
				-D__MOTO_USB_DOWNLOAD__  \
				-DOPENSSL_NO_SOCK        \
				-DOPENSSL_NO_BIO         \
				-DOPENSSL_NO_LHASH       \
				-DOPENSSL_NO_ERR         \
				-DOPENSSL_NO_CMS         \
				-DOPENSSL_NO_ENGINE      \
				-DOPENSSL_NO_EC          \
				-DOPENSSL_NO_ECDSA       \
				-DOPENSSL_NO_ECDH        \
				-DOPENSSL_NO_DSA         \
				-DOPENSSL_NO_DH          \
				-DOPENSSL_NO_LOCKING  \
				-DOPENSSL_SMALL_FOOTPRINT	
endif
ifeq ($(MTK_SEC_MP_KEY), KEY_TYPE_MP)
	DEFINE  := -D$(MTK_PLATFORM) -DKEY_TYPE_MP\
				-D__SECURE_USB_DOWNLOAD__\
				-D__MOTO_USB_DOWNLOAD__  \
				-DOPENSSL_NO_SOCK        \
				-DOPENSSL_NO_BIO         \
				-DOPENSSL_NO_LHASH       \
				-DOPENSSL_NO_ERR         \
				-DOPENSSL_NO_CMS         \
				-DOPENSSL_NO_ENGINE      \
				-DOPENSSL_NO_EC          \
				-DOPENSSL_NO_ECDSA       \
				-DOPENSSL_NO_ECDH        \
				-DOPENSSL_NO_DSA         \
				-DOPENSSL_NO_DH          \
				-DOPENSSL_NO_LOCKING  \
				-DOPENSSL_SMALL_FOOTPRINT	
endif
endif
#DEFINE           = -D$(MTK_PLATFORM)
OBJCFLAGS 	 = --gap-fill=0xff -R .mem_region
AFLAGS_DEBUG 	 = -Wa,-gstabs,
STRIP_SYMBOL	 = -fdata-sections -ffunction-sections
INCLUDE_FILE     =  \
    -I$(MTK_ROOT_OUT)/PRELOADER_OBJ/inc \
    -I$(MTK_PATH_PLATFORM)/src/security/inc \
    -I$(MTK_PATH_PLATFORM)/src/drivers/inc \
    -I$(MTK_PATH_PLATFORM)/src/core/inc \
    -I$(MTK_PATH_PLATFORM)/src/init/inc \
    -I$(MTK_PATH_PLATFORM)/src/security/inc \
    -I$(MTK_ROOT_OUT)/EMIGEN/inc \
    -I$(MTK_PATH_CUSTOM)/inc \
    -I$(D_ROOT)/custom/common/inc \
    -I$(D_ROOT)/inc/$(_CHIP) \
    -I$(MTK_ROOT_CUSTOM_OUT)/kernel/dct \
    -I$(MTK_ROOT_OUT)/PTGEN/common \
    -I$(MTK_ROOT_OUT)/NANDGEN/common \
    -I$(MTK_PATH_PLATFORM)/src/openssl/include

###################################################################
# GCC Compile Options
###################################################################

ifeq ($(CREATE_SEC_LIB),TRUE)

INCLUDE_FILE     +=  \
    -I$(MTK_PATH_PLATFORM)/src/secure_lib/ \
    -I$(MTK_PATH_PLATFORM)/src/secure_lib/inc \
    -I$(MTK_PATH_PLATFORM)/src/secure_lib/crypto_lib \
    -I$(MTK_PATH_PLATFORM)/src/secure_lib/crypto_lib/opt \

# if it's security.lib, we must remove gcc debug message
#C_OPTION	 := -Os -fdata-sections -ffunction-sections -fno-strict-aliasing -fno-common -ffixed-r8 -fno-builtin -ffreestanding -pipe -mno-thumb-interwork -Wstrict-prototypes -march=armv7-a $(DEFINE) -c $(INCLUDE_FILE) -msoft-float -D__ASSEMBLY__  -DPRELOADER_HEAP -mno-unaligned-access
#C_OPTION_OPTIMIZE	 := -Os -fdata-sections -ffunction-sections -fno-strict-aliasing -fno-common -ffixed-r8 -fno-builtin -ffreestanding -pipe -mno-thumb-interwork -Wstrict-prototypes -march=armv7-a $(DEFINE) -c $(INCLUDE_FILE) -msoft-float -D__ASSEMBLY__  -DPRELOADER_HEAP -mno-unaligned-access
#AFLAGS 		 := -c -march=armv7-a -g
#AFLAGS_OPTIMIZE	 := -c -march=armv7-a -g

C_OPTION	 := -Os -fdata-sections -ffunction-sections -fno-strict-aliasing -fno-common -ffixed-r8 -fno-builtin -ffreestanding -pipe -mno-thumb-interwork -Wstrict-prototypes -march=armv7-a $(DEFINE) -c $(INCLUDE_FILE) -msoft-float -D__ASSEMBLY__  -DPRELOADER_HEAP -mno-unaligned-access -mfpu=vfp -mfloat-abi=softfp
C_OPTION_OPTIMIZE	 := -Os -fdata-sections -ffunction-sections -fno-strict-aliasing -fno-common -ffixed-r8 -fno-builtin -ffreestanding -pipe -mno-thumb-interwork -Wstrict-prototypes -march=armv7-a $(DEFINE) -c $(INCLUDE_FILE) -msoft-float -D__ASSEMBLY__  -DPRELOADER_HEAP -mno-unaligned-access -mfpu=vfp -mfloat-abi=softfp
AFLAGS 		 := -c -march=armv7-a -g -mfpu=vfp -mfloat-abi=softfp
AFLAGS_OPTIMIZE	 := -c -march=armv7-a -g -mfpu=vfp -mfloat-abi=softfp

else

C_OPTION	    := -Os $(STRIP_SYMBOL) -fno-strict-aliasing -fno-common -ffixed-r8 -fno-builtin -ffreestanding -pipe -Wstrict-prototypes -march=armv7-a $(DEFINE) -c $(INCLUDE_FILE) -msoft-float -D__ASSEMBLY__ -g -mno-unaligned-access
C_OPTION_OPTIMIZE   := -Os $(STRIP_SYMBOL) -fno-strict-aliasing -fno-common -ffixed-r8 -fno-builtin -ffreestanding -pipe -Wstrict-prototypes -march=armv7-a $(DEFINE) -c $(INCLUDE_FILE) -msoft-float -D__ASSEMBLY__ -g -mno-unaligned-access

AFLAGS 		 := -c -march=armv7-a -g -mfpu=vfp -mfloat-abi=softfp
AFLAGS_OPTIMIZE	 := -c -march=armv7-a -g -mfpu=vfp -mfloat-abi=softfp

endif
#//>20121009-14993-Eric Lin

ifeq ($(THUMB_MODE),TRUE)
#thumb
C_OPTION            += -mthumb-interwork -mthumb
C_OPTION_OPTIMIZE   += -mthumb-interwork -mthumb
else
C_OPTION	    += -mno-thumb-interwork
C_OPTION_OPTIMIZE   += -mno-thumb-interwork
endif

MTK_CDEFS := $(PL_MTK_CDEFS)
MTK_ADEFS := $(PL_MTK_ADEFS)

C_OPTION += $(MTK_CFLAGS) $(MTK_CDEFS) $(MTK_INC)
AFLAGS   += $(MTK_AFLAGS)

###################################################################
# gcc link descriptor
###################################################################

ifeq ($(MTK_PLATFORM),MT6575)
LDSCRIPT	:= $(MTK_PATH_PLATFORM)/link_descriptor_6575.ld
else
ifeq ($(MTK_PROJECT),mt6577_fpga)
LDSCRIPT	:= $(MTK_PATH_PLATFORM)/link_descriptor_6577_fpga.ld
else
LDSCRIPT	:= $(MTK_PATH_PLATFORM)/link_descriptor_6577.ld
endif
endif

LINKFILE	:= $(LD)
LINK		:= $(LINKFILE) -Bstatic -T $(LDSCRIPT) --gc-sections


###################################################################
# Object File
###################################################################

export All_OBJS
