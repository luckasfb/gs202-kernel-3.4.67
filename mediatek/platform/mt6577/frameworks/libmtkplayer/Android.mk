LOCAL_PATH:= $(call my-dir)

#
# libmtkplayer
#

ifneq ($(strip $(BOARD_USES_GENERIC_AUDIO)),true)

#ifneq ($(strip $(HAVE_MATV_FEATURE))_$(strip $(MTK_FM_SUPPORT)), no_no)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:=               \
    mATVAudioPlayer.cpp       \
    FMAudioPlayer.cpp
    
ifeq ($(strip $(BOARD_USES_MTK_AUDIO)),true)
    LOCAL_SRC_FILES+= \
    VibSpkAudioPlayer.cpp
endif

ifeq ($(TARGET_OS)-$(TARGET_SIMULATOR),linux-true)
LOCAL_LDLIBS += -ldl -lpthread
endif

#This is for customization
LOCAL_SHARED_LIBRARIES :=     \
	libcutils             \
	libutils              \
	libbinder             \
	libmedia              \
	libaudiosetting       \
	libandroid_runtime     
	
LOCAL_SHARED_LIBRARIES += libaudio.primary.default

#LOCAL_CFLAGS += -DMTK_MATV_SUPPORT
#LOCAL_CFLAGS += -DMTK_FM_SUPPORT

ifneq ($(TARGET_SIMULATOR),true)
LOCAL_SHARED_LIBRARIES += libdl
endif

LOCAL_C_INCLUDES :=  \
	$(JNI_H_INCLUDE)                                                \
  $(TOP)/frameworks/av/include                                  \
  $(TOP)/frameworks/av/include/media                              \
  $(TOP)/$(MTK_PATH_SOURCE)/frameworks-ext/av/include       \
	$(TOP)/$(MTK_PATH_SOURCE)/frameworks-ext/av/include/media       \
	$(TOP)/external                                                 \
	$(TOP)/$(MTK_PATH_SOURCE)/external/matvctrl                      \
	$(TOP)/$(MTK_PATH_SOURCE)/external/AudioCompensationFilter \
	$(TOP)/$(MTK_PATH_SOURCE)/external/audiodcremoveflt
	
LOCAL_C_INCLUDES += \
  $(TOP)/$(MTK_PATH_PLATFORM)/hardware/audio/aud_drv \
  $(TOP)/$(MTK_PATH_PLATFORM)/hardware/audio/LAD \
  $(TOP)/$(MTK_PATH_PLATFORM)/hardware/audio \
  $(call include-path-for, audio-utils) \
  $(call include-path-for, audio-effects)
  
ifeq ($(strip $(BOARD_USES_MTK_AUDIO)),true)
LOCAL_C_INCLUDES += $(MTK_ROOT_CUSTOM_OUT)/hal/audioflinger/audio
endif  

$(warning Value of MTK_ROOT_CUSTOM_OUT is '$(MTK_ROOT_CUSTOM_OUT)')

ifeq ($(MTK_VIBSPK_SUPPORT),yes)
  LOCAL_CFLAGS += -DMTK_VIBSPK_SUPPORT
endif
LOCAL_MODULE:= libmtkplayer

LOCAL_PRELINK_MODULE := no

include $(BUILD_SHARED_LIBRARY)

#endif	
endif
