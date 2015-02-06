/*******************************************************************************
 *
 * Filename:
 * ---------
 * audio_custom_exp.h
 *
 * Project:
 * --------
 *   ALPS
 *
 * Description:
 * ------------
 * This file is the header of audio customization related function or definition.
 *
 * Author:
 * -------
 * ChiPeng
 *
 *============================================================================
 *             HISTORY
 * Below this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *------------------------------------------------------------------------------
 * $Revision:$
 * $Modtime:$
 * $Log:$
 *
 * 02 24 2012 weiguo.li
 * [ALPS00242182] [Need Patch] [Volunteer Patch] customer bootanimation volume
 * .
 *
 * 10 18 2011 weiguo.li
 * [ALPS00081117] [Need Patch] [Volunteer Patch]mark headcompensation filter marcro for device power up
 * .
 *
 * 10 09 2011 weiguo.li
 * [ALPS00077851] [Need Patch] [Volunteer Patch]LGE audio driver using Voicebuffer code restruct
 * .
 *
 * 04 21 2011 weiguo.li
 * [ALPS00042230] add macro for Camera Shutter Sound
 * .
 *
 * 06 29 2010 chipeng.chang
 * [ALPS00120725][Phone sound] The ringer volume is not high enough when setting ringer volume 
 * update for audio customization for volume base level.
 *
 * 05 26 2010 chipeng.chang
 * [ALPS00002287][Need Patch] [Volunteer Patch] ALPS.10X.W10.11 Volunteer patch for audio paramter
 * modify for Audio parameter
 *
 * 05 11 2010 chipeng.chang
 * [ALPS00002041]Audio Customization
 * add aduio user space customization
 *
 *
 * Mar 15 2010 mtk02308
 *    Init Audio_custom_exp.h
 *
 *
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/
#ifndef AUDIO_CUSTOM_EXP_H
#define AUDIO_CUSTOM_EXP_H

namespace android {


#ifndef DEVICE_MAX_VOLUME
#define DEVICE_MAX_VOLUME      (12)
#endif

#ifndef DEVICE_MIN_VOLUME
#define DEVICE_MIN_VOLUME      (-32)
#endif

#ifndef DEVICE_VOICE_MAX_VOLUME
#define DEVICE_VOICE_MAX_VOLUME      (12)
#endif

#ifndef DEVICE_VOICE_MIN_VOLUME
#define DEVICE_VOICE_MIN_VOLUME      (-32)
#endif

#define ENABLE_AUDIO_COMPENSATION_FILTER  //Define this will enable audio compensation filter for loudspeaker
                                          //Please see ACF Document for detail
                                          
#define ENABLE_AUDIO_DRC_SPEAKER  //Define this will enable DRC for loudspeaker                                          
                                          
#define ENABLE_HEADPHONE_COMPENSATION_FILTER  //Define this will enable headphone compensation filter
                                          //Please see HCF Document for detail
                                          
//#define FORCE_CAMERA_SHUTTER_SOUND_AUDIBLE   //WARNING: this macro thakes no effect now, please change the property value
                                               //ro.camera.sound.forced=1 to take effect.
                                               //the property is defined in alps\mediatek\config\YOUR_PROJECT\system.prop                                         

//#define ENABLE_STEREO_SPEAKER
                                          // if define Stereo speaker , speaker output will not do stero to mono, keep in stereo format
                                          //because stereo output can apply on more than 1 speaker.

//#define ALL_USING_VOICEBUFFER_INCALL     //Define this will enable Voice  to VoiceBuffer when using speaker and headphone in incall mode.

// when defein AUDIO_HQA_SUPPORT audioflinger will use first active stream samplerate as hardware setting.
// generally is is only use for verifying hardware 
//#define AUDIO_HQA_SUPPORT

#ifdef ENABLE_HEADPHONE_COMPENSATION_FILTER
    #define HEADPHONE_COMPENSATION_FLT_MODE (4)
#endif

#define ENABLE_AUDIO_SW_STEREO_TO_MONO    //Define this will enable SW stereo to mono on LCH & RCH
                                          //If not define this, HW stereo to mono (only LCH) will be applied

#define USE_REFMIC_IN_LOUDSPK (0)         //(1)->Use Ref Mic as main mic; (0)->Use original main mic.

#define ENABLE_HIGH_SAMPLERATE_RECORD

#define BOOT_ANIMATION_VOLUME (0.25)	// adjust boot animation volume. the valume is from 0 to 1.


}

#endif
