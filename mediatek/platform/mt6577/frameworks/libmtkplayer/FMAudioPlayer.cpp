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

//#define LOG_NDEBUG 0
#define LOG_TAG "FMPlayer"
#include "utils/Log.h"
#include "cutils/xlog.h"

#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <system/audio.h>

/*extern "C" {
#include "kal_release.h"
}*/

#include <binder/IServiceManager.h>

#include <AudioSystem.h>
#include "AudioYusuDef.h"
#include "AudioI2S.h"
#include "FMAudioPlayer.h"
#include <linux/rtpm_prio.h>

#include "audio_custom_exp.h"

//#define FM_AUDIO_FILELOG

#ifdef HAVE_GETTID
static pid_t myTid() { return gettid(); }
#else
static pid_t myTid() { return getpid(); }
#endif

static long long getTimeMs()
{
    struct timeval t1;
    long long ms;

    gettimeofday(&t1, NULL);
    ms = t1.tv_sec * 1000LL + t1.tv_usec / 1000;

    return ms;
}

// ----------------------------------------------------------------------------

extern const int fm_use_analog_input;//from libaudiosetting.so
extern const int fm_chip_519x;//from libaudiosetting.so


namespace android {

// ----------------------------------------------------------------------------

//#define FAKE_FM
#define sineTable48KSIZE 480
#define sineTable32KSIZE 320
//sine table for simulation only
#ifdef FAKE_FM

static const uint16_t sineTable48K[480] = {
    0x1   ,0x0,0xbd4   ,0xbd4,0x1773   ,0x1774,0x22ae   ,0x22ad,
    0x2d4e   ,0x2d50,0x372a   ,0x372a,0x4013   ,0x4013,0x47e3   ,0x47e3,
    0x4e79   ,0x4e79,0x53b8   ,0x53b8,0x5787   ,0x5787,0x59d6   ,0x59d7,
    0x5a9d   ,0x5a9d,0x59d6   ,0x59d7,0x5786   ,0x5787,0x53b7   ,0x53b7,
    0x4e79   ,0x4e7a,0x47e4   ,0x47e3,0x4012   ,0x4013,0x372a   ,0x372a,
    0x2d4f   ,0x2d50,0x22ac   ,0x22ad,0x1773   ,0x1774,0xbd4   ,0xbd3,
    0x0   ,0x0,0xf42c   ,0xf42c,0xe88c   ,0xe88c,0xdd53   ,0xdd52,
    0xd2b1   ,0xd2b1,0xc8d6   ,0xc8d6,0xbfed   ,0xbfed,0xb81d   ,0xb81c,
    0xb187   ,0xb186,0xac49   ,0xac49,0xa87a   ,0xa879,0xa629   ,0xa62a,
    0xa563   ,0xa563,0xa629   ,0xa629,0xa879   ,0xa879,0xac49   ,0xac49,
    0xb186   ,0xb187,0xb81c   ,0xb81c,0xbfed   ,0xbfed,0xc8d6   ,0xc8d6,
    0xd2b2   ,0xd2b2,0xdd53   ,0xdd52,0xe88d   ,0xe88c,0xf42c   ,0xf42c,
    0xffff   ,0xffff,0xbd4   ,0xbd3,0x1774   ,0x1774,0x22ad   ,0x22ad,
    0x2d4e   ,0x2d4f,0x372a   ,0x3729,0x4013   ,0x4013,0x47e3   ,0x47e3,
    0x4e7a   ,0x4e79,0x53b7   ,0x53b8,0x5787   ,0x5786,0x59d7   ,0x59d7,
    0x5a9e   ,0x5a9d,0x59d7   ,0x59d7,0x5787   ,0x5786,0x53b8   ,0x53b7,
    0x4e79   ,0x4e7a,0x47e3   ,0x47e4,0x4013   ,0x4013,0x3729   ,0x372a,
    0x2d4f   ,0x2d4f,0x22ad   ,0x22ad,0x1774   ,0x1774,0xbd4   ,0xbd4,
    0x0   ,0x1,0xf42d   ,0xf42c,0xe88c   ,0xe88b,0xdd53   ,0xdd53,
    0xd2b1   ,0xd2b2,0xc8d7   ,0xc8d6,0xbfed   ,0xbfed,0xb81c   ,0xb81c,
    0xb187   ,0xb186,0xac48   ,0xac48,0xa879   ,0xa879,0xa629   ,0xa629,
    0xa563   ,0xa563,0xa629   ,0xa62a,0xa879   ,0xa879,0xac49   ,0xac49,
    0xb186   ,0xb187,0xb81d   ,0xb81c,0xbfed   ,0xbfed,0xc8d7   ,0xc8d6,
    0xd2b1   ,0xd2b1,0xdd53   ,0xdd54,0xe88c   ,0xe88c,0xf42c   ,0xf42c,
    0x0   ,0xffff,0xbd4   ,0xbd4,0x1773   ,0x1773,0x22ad   ,0x22ae,
    0x2d4f   ,0x2d4f,0x3729   ,0x372a,0x4013   ,0x4013,0x47e4   ,0x47e4,
    0x4e7a   ,0x4e79,0x53b7   ,0x53b7,0x5787   ,0x5788,0x59d6   ,0x59d6,
    0x5a9e   ,0x5a9d,0x59d7   ,0x59d7,0x5787   ,0x5786,0x53b8   ,0x53b7,
    0x4e7a   ,0x4e79,0x47e4   ,0x47e4,0x4013   ,0x4013,0x3729   ,0x372a,
    0x2d4f   ,0x2d4f,0x22ad   ,0x22ad,0x1774   ,0x1774,0xbd4   ,0xbd4,
    0x0   ,0xffff,0xf42c   ,0xf42c,0xe88c   ,0xe88d,0xdd52   ,0xdd53,
    0xd2b1   ,0xd2b1,0xc8d7   ,0xc8d6,0xbfed   ,0xbfed,0xb81c   ,0xb81d,
    0xb186   ,0xb186,0xac48   ,0xac49,0xa879   ,0xa879,0xa628   ,0xa629,
    0xa563   ,0xa563,0xa629   ,0xa62a,0xa879   ,0xa879,0xac48   ,0xac49,
    0xb186   ,0xb187,0xb81c   ,0xb81d,0xbfed   ,0xbfed,0xc8d6   ,0xc8d6,
    0xd2b1   ,0xd2b2,0xdd53   ,0xdd53,0xe88b   ,0xe88c,0xf42c   ,0xf42c,
    0xffff   ,0xffff,0xbd3   ,0xbd4,0x1774   ,0x1774,0x22ad   ,0x22ad,
    0x2d4f   ,0x2d4f,0x3729   ,0x372a,0x4012   ,0x4013,0x47e3   ,0x47e4,
    0x4e7a   ,0x4e7a,0x53b8   ,0x53b7,0x5787   ,0x5787,0x59d7   ,0x59d7,
    0x5a9d   ,0x5a9d,0x59d6   ,0x59d7,0x5787   ,0x5786,0x53b7   ,0x53b7,
    0x4e7a   ,0x4e79,0x47e4   ,0x47e4,0x4013   ,0x4013,0x372a   ,0x372a,
    0x2d4f   ,0x2d4f,0x22ad   ,0x22ad,0x1774   ,0x1774,0xbd4   ,0xbd3,
    0x0   ,0xffff,0xf42c   ,0xf42c,0xe88c   ,0xe88c,0xdd53   ,0xdd53,
    0xd2b2   ,0xd2b1,0xc8d7   ,0xc8d6,0xbfed   ,0xbfed,0xb81d   ,0xb81d,
    0xb187   ,0xb187,0xac48   ,0xac48,0xa87a   ,0xa879,0xa62a   ,0xa62a,
    0xa562   ,0xa563,0xa629   ,0xa629,0xa879   ,0xa879,0xac49   ,0xac48,
    0xb186   ,0xb186,0xb81d   ,0xb81c,0xbfee   ,0xbfee,0xc8d6   ,0xc8d7,
    0xd2b1   ,0xd2b1,0xdd53   ,0xdd53,0xe88c   ,0xe88c,0xf42c   ,0xf42c,
    0x1   ,0x0,0xbd4   ,0xbd4,0x1774   ,0x1774,0x22ac   ,0x22ae,
    0x2d4e   ,0x2d4f,0x372a   ,0x372a,0x4013   ,0x4013,0x47e4   ,0x47e4,
    0x4e79   ,0x4e79,0x53b8   ,0x53b7,0x5787   ,0x5787,0x59d7   ,0x59d7,
    0x5a9d   ,0x5a9c,0x59d6   ,0x59d7,0x5787   ,0x5787,0x53b8   ,0x53b7,
    0x4e78   ,0x4e7a,0x47e3   ,0x47e4,0x4013   ,0x4013,0x3729   ,0x3729,
    0x2d4f   ,0x2d4f,0x22ae   ,0x22ad,0x1774   ,0x1774,0xbd4   ,0xbd4,
    0x0   ,0x0,0xf42c   ,0xf42c,0xe88c   ,0xe88d,0xdd53   ,0xdd53,
    0xd2b1   ,0xd2b1,0xc8d7   ,0xc8d6,0xbfee   ,0xbfed,0xb81c   ,0xb81c,
    0xb187   ,0xb187,0xac49   ,0xac49,0xa879   ,0xa879,0xa629   ,0xa629,
    0xa563   ,0xa563,0xa628   ,0xa629,0xa879   ,0xa87a,0xac49   ,0xac48,
    0xb186   ,0xb186,0xb81d   ,0xb81d,0xbfec   ,0xbfed,0xc8d6   ,0xc8d6,
    0xd2b1   ,0xd2b1,0xdd54   ,0xdd53,0xe88d   ,0xe88b,0xf42b   ,0xf42c
};

static const uint16_t sineTable[320] = {
0x0000, 0x0A03, 0x13C7, 0x1D0E, 0x259E, 0x2D41, 0x33C7, 0x3906,
0x3CDE, 0x3F36, 0x4000, 0x3F36, 0x3CDE, 0x3906, 0x33C7, 0x2D41,
0x259E, 0x1D0E, 0x13C7, 0x0A03, 0x0000, 0xF5FD, 0xEC39, 0xE2F2,
0xDA62, 0xD2BF, 0xCC39, 0xC6FA, 0xC322, 0xC0CA, 0xC000, 0xC0CA,
0xC322, 0xC6FA, 0xCC39, 0xD2BF, 0xDA62, 0xE2F2, 0xEC39, 0xF5FD,
0xFFFF, 0x0A03, 0x13C7, 0x1D0E, 0x259E, 0x2D41, 0x33C7, 0x3906,
0x3CDE, 0x3F36, 0x4000, 0x3F36, 0x3CDE, 0x3906, 0x33C7, 0x2D41,
0x259E, 0x1D0E, 0x13C7, 0x0A03, 0x0000, 0xF5FD, 0xEC39, 0xE2F2,
0xDA62, 0xD2BF, 0xCC39, 0xC6FA, 0xC322, 0xC0CA, 0xC000, 0xC0CA,
0xC322, 0xC6FA, 0xCC39, 0xD2BF, 0xDA62, 0xE2F2, 0xEC39, 0xF5FD,
0xFFFF, 0x0A03, 0x13C7, 0x1D0E, 0x259E, 0x2D41, 0x33C7, 0x3906,
0x3CDE, 0x3F36, 0x4000, 0x3F36, 0x3CDE, 0x3906, 0x33C7, 0x2D41,
0x259E, 0x1D0E, 0x13C7, 0x0A03, 0x0000, 0xF5FD, 0xEC39, 0xE2F2,
0xDA62, 0xD2BF, 0xCC39, 0xC6FA, 0xC322, 0xC0CA, 0xC000, 0xC0CA,
0xC322, 0xC6FA, 0xCC39, 0xD2BF, 0xDA62, 0xE2F2, 0xEC39, 0xF5FD,
0xFFFF, 0x0A03, 0x13C7, 0x1D0E, 0x259E, 0x2D41, 0x33C7, 0x3906,
0x3CDE, 0x3F36, 0x4000, 0x3F36, 0x3CDE, 0x3906, 0x33C7, 0x2D41,
0x259E, 0x1D0E, 0x13C7, 0x0A03, 0x0000, 0xF5FD, 0xEC39, 0xE2F2,
0xDA62, 0xD2BF, 0xCC39, 0xC6FA, 0xC322, 0xC0CA, 0xC000, 0xC0CA,
0xC322, 0xC6FA, 0xCC39, 0xD2BF, 0xDA62, 0xE2F2, 0xEC39, 0xF5FD,
0xFFFF, 0x0A03, 0x13C7, 0x1D0E, 0x259E, 0x2D41, 0x33C7, 0x3906,
0x3CDE, 0x3F36, 0x4000, 0x3F36, 0x3CDE, 0x3906, 0x33C7, 0x2D41,
0x259E, 0x1D0E, 0x13C7, 0x0A03, 0x0000, 0xF5FD, 0xEC39, 0xE2F2,
0xDA62, 0xD2BF, 0xCC39, 0xC6FA, 0xC322, 0xC0CA, 0xC000, 0xC0CA,
0xC322, 0xC6FA, 0xCC39, 0xD2BF, 0xDA62, 0xE2F2, 0xEC39, 0xF5FD,
0xFFFF, 0x0A03, 0x13C7, 0x1D0E, 0x259E, 0x2D41, 0x33C7, 0x3906,
0x3CDE, 0x3F36, 0x4000, 0x3F36, 0x3CDE, 0x3906, 0x33C7, 0x2D41,
0x259E, 0x1D0E, 0x13C7, 0x0A03, 0x0000, 0xF5FD, 0xEC39, 0xE2F2,
0xDA62, 0xD2BF, 0xCC39, 0xC6FA, 0xC322, 0xC0CA, 0xC000, 0xC0CA,
0xC322, 0xC6FA, 0xCC39, 0xD2BF, 0xDA62, 0xE2F2, 0xEC39, 0xF5FD,
0xFFFF, 0x0A03, 0x13C7, 0x1D0E, 0x259E, 0x2D41, 0x33C7, 0x3906,
0x3CDE, 0x3F36, 0x4000, 0x3F36, 0x3CDE, 0x3906, 0x33C7, 0x2D41,
0x259E, 0x1D0E, 0x13C7, 0x0A03, 0xFFFF, 0xF5FD, 0xEC39, 0xE2F2,
0xDA62, 0xD2BF, 0xCC39, 0xC6FA, 0xC322, 0xC0CA, 0xC000, 0xC0CA,
0xC322, 0xC6FA, 0xCC39, 0xD2BF, 0xDA62, 0xE2F2, 0xEC39, 0xF5FD,
0xFFFF, 0x0A03, 0x13C7, 0x1D0E, 0x259E, 0x2D41, 0x33C7, 0x3906,
0x3CDE, 0x3F36, 0x4000, 0x3F36, 0x3CDE, 0x3906, 0x33C7, 0x2D41,
0x259E, 0x1D0E, 0x13C7, 0x0A03, 0x0000, 0xF5FD, 0xEC39, 0xE2F2,
0xDA62, 0xD2BF, 0xCC39, 0xC6FA, 0xC322, 0xC0CA, 0xC000, 0xC0CA,
0xC322, 0xC6FA, 0xCC39, 0xD2BF, 0xDA62, 0xE2F2, 0xEC39, 0xF5FD,
};
#endif

// Define I2S related functions to make switch between fake and real easy

static void* I2SGetInstance()
{
    SXLOGD("I2SGetInstance");
#ifdef FAKE_FM
    return (void*)sineTable;              //a fake address, we do not use this handle in fake case
#else
    return (void*)AudioI2S::getInstance();
#endif
}

static void I2SFreeInstance(void *handle)
{
    SXLOGD("I2SFreeInstance");
#ifdef FAKE_FM
    return;              //Just return
#else
    AudioI2S* i2sHandle = (AudioI2S*)handle;
    i2sHandle->freeInstance();
#endif
}

static uint32 I2SOpen(void* handle)
{
    SXLOGD("I2SOpen");
#ifdef FAKE_FM
    return 1;                         //a fake id
#else
    AudioI2S* i2sHandle = (AudioI2S*)handle;
    return i2sHandle->open();
#endif
}


static bool I2SSet(void* handle, I2STYPE type)
{
    SXLOGD("I2SSet");
#ifdef FAKE_FM
    return 1;                         //always return ture
#else
    AudioI2S* i2sHandle = (AudioI2S*)handle;
    return i2sHandle->set(type);
#endif
}


static bool I2SClose(void* handle, uint32_t Identity)
{
    SXLOGD("I2SClose");
#ifdef FAKE_FM
    return 1;                         //always return ture
#else
    AudioI2S* i2sHandle = (AudioI2S*)handle;
    return i2sHandle->close(Identity);
#endif
}

static bool I2SStart(void* handle, uint32_t Identity,I2STYPE type)
{
    SXLOGD("I2SStart");
#ifdef FAKE_FM
    return 1;                         //always return ture
#else
    AudioI2S* i2sHandle = (AudioI2S*)handle;
    return i2sHandle->start(Identity,type);
#endif
}

static bool I2SStop(void* handle, uint32_t Identity,I2STYPE type)
{
    SXLOGD("I2SStop");
#ifdef FAKE_FM
    return 1;                         //always return ture
#else
    AudioI2S* i2sHandle = (AudioI2S*)handle;
    return i2sHandle->stop(Identity,type);
#endif
}

static uint32 I2SGetReadBufferSize(void* handle)
{
#ifdef FAKE_FM
    return 384 * sizeof(uint16_t) * 2;        // 2 x sine table size
#else
    AudioI2S* i2sHandle = (AudioI2S*)handle;
    return i2sHandle->GetReadBufferSize();
#endif
}
static uint32 I2SSampleRate(void* handle)
{
    SXLOGD("I2SSampleRate");
#ifdef FAKE_FM
    return 48000;                         //a fake id
#else
    AudioI2S* i2sHandle = (AudioI2S*)handle;
    return i2sHandle->samplerate();
#endif
}
static uint32 I2SRead(void* handle, uint32_t Identity,void* buffer, uint32 buffersize)
{
#ifdef FAKE_FM
    usleep(1000);
    int sineTableSize = sineTable48KSIZE*sizeof(uint16_t);
    char * ptr = (char*)buffer;
    memcpy(ptr, sineTable48K, sineTableSize);
    ptr += sineTableSize;
    memcpy(ptr, sineTable48K, sineTableSize);
    ptr += sineTableSize;

    return sineTableSize * 2;

#else
    AudioI2S* i2sHandle = (AudioI2S*)handle;
    return i2sHandle->read(Identity, buffer, buffersize);
#endif

}

// TODO: Determine appropriate return codes
static status_t ERROR_NOT_OPEN = -1;
static status_t ERROR_OPEN_FAILED = -2;
static status_t ERROR_ALLOCATE_FAILED = -4;
static status_t ERROR_NOT_SUPPORTED = -8;
static status_t ERROR_NOT_READY = -16;
static status_t ERROR_START_FAILED = -32;
static status_t ERROR_STOP_FAILED = -64;
static status_t STATE_INIT = 0;
static status_t STATE_ERROR = 1;
static status_t STATE_OPEN = 2;

String8  ANALOG_FM_ENABLE =(String8)("AudioSetFmEnable=1");
String8  ANALOG_FM_DISABLE =(String8)("AudioSetFmEnable=0");

String8  DIGITAL_FM_ENABLE =(String8)("AudioSetFmDigitalEnable=1");
String8  DIGITAL_FM_DISABLE =(String8)("AudioSetFmDigitalEnable=0");


FMAudioPlayer::FMAudioPlayer() :
    mAudioBuffer(NULL), mPlayTime(-1), mDuration(-1), mState(STATE_ERROR),
    mStreamType(AUDIO_STREAM_FM),
    mExit(false), mPaused(false), mRender(false), mRenderTid(-1), mI2Sid(0), mI2Sdriver(NULL), mI2SStartFlag(0),mDataType(-1)
{
    SXLOGD("[%d]FMAudioPlayer constructor\n", mI2Sid);
    if (fm_use_analog_input == 1)
    {
    	SXLOGD("FM use analog input");
    }

    else if(fm_use_analog_input == 0)
    {
       mI2Sdriver = I2SGetInstance();
       if ( mI2Sdriver == NULL ){
        SXLOGE("I2S driver doesn't exists\n");
       }
    }

    mMutePause = 0;
}

void FMAudioPlayer::onFirstRef()
{
    SXLOGD("onFirstRef");

    // create playback thread
    Mutex::Autolock l(mMutex);

    if (fm_use_analog_input == 1)
   	{
      SXLOGD("FMAudioPlayer use analog input - onFirstRef");
      mRenderTid =1;
      mState = STATE_INIT;
    }
    else if (fm_use_analog_input == 0)
    {
       createThreadEtc(renderThread, this, "FM audio player", ANDROID_PRIORITY_AUDIO);
       mCondition.waitRelative(mMutex,seconds (3));
       if (mRenderTid > 0) {
        SXLOGD("[%d]render thread(%d) started", mI2Sid, mRenderTid);
        mState = STATE_INIT;
       }
    }
}

status_t FMAudioPlayer::initCheck()
{
    if (mState != STATE_ERROR) return NO_ERROR;
    return ERROR_NOT_READY;
}

FMAudioPlayer::~FMAudioPlayer() {
    SXLOGD("[%d]FMAudioPlayer destructor\n", mI2Sid);
    release();
    if (fm_use_analog_input == 1)
      SXLOGD("FMAudioPlayer use analog input - destructor end\n");

    else if (fm_use_analog_input == 0)
    {
      //free I2S instance
      I2SFreeInstance(mI2Sdriver);
      mI2Sdriver = NULL;
      SXLOGD("[%d]FMAudioPlayer destructor end\n", mI2Sid);
    }
}

status_t FMAudioPlayer::setDataSource(
        const char* path, const KeyedVector<String8, String8> *)
{
    SXLOGD("FMAudioPlayer setDataSource path=%s \n",path);
    return setdatasource(path, -1, 0, 0x7ffffffffffffffLL); // intentionally less than LONG_MAX
}

status_t FMAudioPlayer::setDataSource(int fd, int64_t offset, int64_t length)
{
    SXLOGD("FMAudioPlayer setDataSource offset=%d, length=%d \n",((int)offset),((int)length));
    return setdatasource(NULL, fd, offset, length);
}


status_t FMAudioPlayer::setdatasource(const char *path, int fd, int64_t offset, int64_t length)
{
    SXLOGD("[%d]setdatasource",mI2Sid);

    // file still open?
    Mutex::Autolock l(mMutex);
    if (mState == STATE_OPEN) {
        reset_nosync();
    }

    if (fm_use_analog_input == 1)
    {
      mState = STATE_OPEN;
      return NO_ERROR;
     }
    else if (fm_use_analog_input == 0)
    {
      //already opend
      if (mI2Sdriver && mI2Sid!=0) {
    	mState = STATE_OPEN;
       return NO_ERROR;
      }

      // get ID
      mI2Sid = I2SOpen(mI2Sdriver);
      if ( mI2Sid == 0 ){
        SXLOGE("I2S driver get ID fail\n");
        mState = STATE_ERROR;
        return ERROR_OPEN_FAILED;
      }
      //set FM
      /*
      if ( !I2SSet(mI2Sdriver,FMRX) ){
        SXLOGE("I2S driver set MATV fail\n");
        mState = STATE_ERROR;
        return ERROR_OPEN_FAILED;
      }*/
      SXLOGD("Finish initial I2S driver, instance:%p, ID:%d", mI2Sdriver, mI2Sid);

      mState = STATE_OPEN;
      return NO_ERROR;

    }

    return NO_ERROR;
}

status_t FMAudioPlayer::prepare()
{
    SXLOGD("[%d]prepare\n", mI2Sid);

    if (mState != STATE_OPEN ) {
        SXLOGE("prepare ERROR_NOT_OPEN \n");
        return ERROR_NOT_OPEN;
    }
    return NO_ERROR;
}

status_t FMAudioPlayer::prepareAsync() {
    SXLOGD("[%d]prepareAsync\n", mI2Sid);

    // can't hold the lock here because of the callback
    // it's safe because we don't change state
    if (mState != STATE_OPEN ) {
        sendEvent(MEDIA_ERROR);
        SXLOGD("prepareAsync sendEvent(MEDIA_ERROR) \n");
        return NO_ERROR;
    }
    sendEvent(MEDIA_PREPARED);
    return NO_ERROR;
}

status_t FMAudioPlayer::start()
{
    SXLOGD("[%d]start\n", mI2Sid);
    Mutex::Autolock l(mMutex);
    if (mState != STATE_OPEN) {
        SXLOGE("start ERROR_NOT_OPEN \n");
        return ERROR_NOT_OPEN;
    }

    if (fm_use_analog_input == 1)
    {
      if (fm_chip_519x == 1)
      {
       {
       int a = 7393;
       int b = 739;
       int c = 73939;
       int in, out;
       //sp<IATVCtrlService> spATVCtrlService;
       sp<IServiceManager> sm = defaultServiceManager();
       sp<IBinder> binder;
       do{
          binder = sm->getService(String16("media.ATVCtrlService"));
          if (binder != 0)
             break;

          SXLOGW("ATVCtrlService not published, waiting...");
          usleep(500000); // 0.5 s
       } while(true);
       spATVCtrlService = interface_cast<IATVCtrlService>(binder);
       in = (int)&a;
       out = spATVCtrlService->ATVCS_matv_set_parameterb(in);
       if ( out != (in+a)*b + c ) {
          SXLOGD("Set Parameberb failed %d", out);
          return ERROR_NOT_OPEN;
        }
       }

       ///if(spATVCtrlService)
       {
        SXLOGE("FM Set 5192 Line in");
        spATVCtrlService->ATVCS_matv_set_chipdep(190,3);
       }
      }
     AudioSystem::setParameters (0,ANALOG_FM_ENABLE);

     mPaused = false;
     mRender = true;

    }
    else if (fm_use_analog_input == 0)
    {

      if ( mMutePause == true) {
       mMutePause = false;
       mPaused = false;
       mAudioSink->setVolume (1.0,1.0);
      }

      // Start I2S driver
      if(!mI2SStartFlag) {
          if (fm_chip_519x == 1){
              if(!I2SStart(mI2Sdriver,mI2Sid,MATV)){
                  SXLOGE("I2S start fialed");
                  reset_nosync();
                  return ERROR_START_FAILED;
              }
              mDataType = MATV;
          }

          else{
              if(!I2SStart(mI2Sdriver,mI2Sid,FMRX)){
                  SXLOGE("I2S start fialed");
                  reset_nosync();
                  return ERROR_START_FAILED;
              }
              mDataType = FMRX;
          }
          mI2SStartFlag = 1;
      }
     mPaused = false;
     mRender = true;

     //AudioSystem::setParameters (0,DIGITAL_FM_ENABLE);

     // wake up render thread
     SXLOGD("start wakeup render thread\n");
     mCondition.signal();
     }

    return NO_ERROR;
}

status_t FMAudioPlayer::stop()
{
    SXLOGD("[%d]stop\n",mI2Sid);
    Mutex::Autolock l(mMutex);
    if (mState != STATE_OPEN) {
        SXLOGE("stop ERROR_NOT_OPEN \n");
        return ERROR_NOT_OPEN;
    }

    if (fm_use_analog_input == 1)
    AudioSystem::setParameters (0,ANALOG_FM_DISABLE);

    else if (fm_use_analog_input == 0)
    {
      // stop I2S driver
      if(mI2SStartFlag == 1){                    //only when I2S is open
       if(!I2SStop(mI2Sdriver,mI2Sid,(I2STYPE)mDataType)){
    	   SXLOGE("I2S stop fialed");
    	   reset_nosync();
    	   return ERROR_STOP_FAILED;
       }
       mI2SStartFlag = 0;
      }
     AudioSystem::setParameters (0,DIGITAL_FM_DISABLE);

    }
    mPaused = true;
    mRender = false;
    return NO_ERROR;
}

status_t FMAudioPlayer::seekTo(int position)
{
    SXLOGD("[%d]seekTo %d\n", position, mI2Sid);
    Mutex::Autolock l(mMutex);
    return NO_ERROR;
}

status_t FMAudioPlayer::pause()
{
    SXLOGD("[%d]pause\n",mI2Sid);
    Mutex::Autolock l(mMutex);
    if (mState != STATE_OPEN) {
        SXLOGD("pause ERROR_NOT_OPEN \n");
        return ERROR_NOT_OPEN;
    }
    SXLOGD("pause got lock\n");
    if (fm_use_analog_input == 1)
    {
        if ( mMutePause == false) {
            mMutePause = true;
            AudioSystem::setParameters (0,ANALOG_FM_DISABLE);
        }
    }
    else if (fm_use_analog_input == 0)
    {
        if( mMutePause == false) {
            mMutePause = true;
            mAudioSink->setVolume (0.0 , 0.0);
        }
    }
    mPaused = true;
    return NO_ERROR;
}

bool FMAudioPlayer::isPlaying()
{
    SXLOGD("[%d]isPlaying\n",mI2Sid);
    if (mState == STATE_OPEN) {
        if(mPaused)
            return false;
        else
            return mRender;
    }
    return false;
}

status_t FMAudioPlayer::getCurrentPosition(int* position)
{
    SXLOGD("[%d]getCurrentPosition always return 0\n", mI2Sid);
    Mutex::Autolock l(mMutex);
    if (mState != STATE_OPEN) {
        SXLOGD("getCurrentPosition(): file not open");
        return ERROR_NOT_OPEN;
    }
    *position = 0;
    return NO_ERROR;
}

status_t FMAudioPlayer::getDuration(int* duration)
{
    Mutex::Autolock l(mMutex);
    if (mState != STATE_OPEN) {
        SXLOGD("getDuration ERROR_NOT_OPEN \n");
        return ERROR_NOT_OPEN;
    }

    *duration = 1000;
    SXLOGD("[%d]getDuration duration, always return 0 \n",mI2Sid);
    return NO_ERROR;
}

status_t FMAudioPlayer::release()
{
    SXLOGD("[%d]release\n",mI2Sid);

    int ret =0;
    int count = 100;
    SXLOGD("release mMutex.tryLock ()");
    do{
        ret = mMutex.tryLock ();
        if(ret){
            SXLOGW("FMAudioPlayer::release() mMutex return ret = %d",ret);
            usleep(20*1000);
            count --;
        }
    }while(ret && count);  // only cannot lock

    reset_nosync();
    if (fm_use_analog_input == 1){
        SXLOGD("FMAudioPlayer use analog input");
    }
    else if (fm_use_analog_input == 0)
    {
        // TODO: timeout when thread won't exit, wait for render thread to exit
        if (mRenderTid > 0) {
            mExit = true;
            SXLOGD("release signal \n");
            mCondition.signal();
            SXLOGD("release wait \n");
            mCondition.waitRelative(mMutex,seconds (3));
        }
    }
    mMutex.unlock ();
    return NO_ERROR;
}

status_t FMAudioPlayer::reset()
{
    SXLOGD("[%d]reset\n", mI2Sid);
    Mutex::Autolock l(mMutex);
    return reset_nosync();
}

// always call with lock held
status_t FMAudioPlayer::reset_nosync()
{
    SXLOGD("[%d]reset_nosync start\n",mI2Sid);

    if (fm_use_analog_input == 1){
        SXLOGD("FMAudioPlayer use analog input");
        AudioSystem::setParameters (0,ANALOG_FM_DISABLE);//Add by Changqing
    }
    else if (fm_use_analog_input == 0)
    {
        // ToDo: close I2S driver
        if (mI2Sdriver && mI2Sid!=0) {
            I2SStop(mI2Sdriver,mI2Sid,(I2STYPE)mDataType);
            I2SClose(mI2Sdriver,mI2Sid);
        }
        mI2Sid = 0;
     AudioSystem::setParameters (0,DIGITAL_FM_DISABLE);//Add by Changqing
    }

    mI2SStartFlag = 0;
    mState = STATE_ERROR;
    mPlayTime = -1;
    mDuration = -1;
    mPaused = false;
    mRender = false;
    SXLOGD("[%d]reset_nosync end\n",mI2Sid);
    return NO_ERROR;
}

status_t FMAudioPlayer::setLooping(int loop)
{
    SXLOGD("[%d]setLooping, do nothing \n",mI2Sid);
    return NO_ERROR;
}

#define FM_AUDIO_CHANNEL_NUM      2

status_t FMAudioPlayer::createOutputTrack() {
    // base on configuration define samplerate .
    int FM_AUDIO_SAMPLING_RATE;

    if (fm_use_analog_input == 1)
    {
      if (fm_chip_519x == 1)
        FM_AUDIO_SAMPLING_RATE = 32000;
      else
        FM_AUDIO_SAMPLING_RATE = 48000;
    }
    else
    {
      FM_AUDIO_SAMPLING_RATE = I2SSampleRate(mI2Sdriver);
    }

    
    SXLOGD("Create AudioTrack object: rate=%d, channels=%d\n", FM_AUDIO_SAMPLING_RATE, FM_AUDIO_CHANNEL_NUM);

    // open audio track
    if (mAudioSink->open(FM_AUDIO_SAMPLING_RATE, FM_AUDIO_CHANNEL_NUM, AUDIO_CHANNEL_OUT_STEREO, AUDIO_FORMAT_PCM_16_BIT, 3) != NO_ERROR) {
        SXLOGE("mAudioSink open failed");
        return ERROR_OPEN_FAILED;
    }
    return NO_ERROR;
}

int FMAudioPlayer::renderThread(void* p) {
    return ((FMAudioPlayer*)p)->render();
}

//#define AUDIOBUFFER_SIZE 4096
int FMAudioPlayer::render() {
    int result = -1;
    int temp;
    int current_section = 0;
    bool audioStarted = false;
    bool firstOutput = false;
    int t_result = -1;
    int bufSize = 0;
    int lastTime = 0;
    int thisTime = 0;
    int dataCount = 0;
    int frameCount = 0;


#ifdef FM_AUDIO_FILELOG
   FILE *fp;
   fp = fopen("sdcard/test.pcm","wb");
   SXLOGD("fp:%d", fp);
#endif

    bufSize = I2SGetReadBufferSize(mI2Sdriver);
    SXLOGD("got buffer size = %d", bufSize);
    mAudioBuffer = new char[bufSize*2];
    mDummyBuffer = new char[bufSize*2];
    memset(mDummyBuffer, 0, bufSize);

    SXLOGD("mAudioBuffer: %p \n",mAudioBuffer);
    if (!mAudioBuffer) {
        SXLOGD("mAudioBuffer allocate failed\n");
        goto threadExit;
    }
    // if set prority false , force to set priority
    if(t_result == -1)
    {
       struct sched_param sched_p;
       sched_getparam(0, &sched_p);
       sched_p.sched_priority = RTPM_PRIO_FM_AUDIOPLAYER ;
       if(0 != sched_setscheduler(0, SCHED_RR, &sched_p))
       {
          SXLOGE("[%s] failed, errno: %d", __func__, errno);
       }
       else
       {
          sched_p.sched_priority = RTPM_PRIO_FM_AUDIOPLAYER;
          sched_getparam(0, &sched_p);
          SXLOGD("sched_setscheduler ok, priority: %d", sched_p.sched_priority);
       }
    }

    // let main thread know we're ready
    {
        int ret =0;
        int count = 100;
        SXLOGD("render mMutex.tryLock ()");
        do{
            ret = mMutex.tryLock ();
            if(ret){
                SXLOGW("FMAudioPlayer::release() mMutex return ret = %d",ret);
                usleep(20*1000);
                count --;
            }
        }while(ret && count);  // only cannot lock

        mRenderTid = myTid();
        SXLOGD("[%d]render start mRenderTid=%d\n",mI2Sid, mRenderTid);
        mCondition.signal();
        mMutex.unlock ();
    }

    while (1) {
        long numread = 0;
        {
            Mutex::Autolock l(mMutex);

            // pausing?
            if (mPaused) {
                SXLOGD("render - pause\n");
                if (mAudioSink->ready()){
                	mAudioSink->pause();
                	mAudioSink->flush();
                }
                usleep(300* 1000);
                mRender = false;
                audioStarted = false;
            }

            // nothing to render, wait for client thread to wake us up
            if (!mExit && !mRender) {
                SXLOGD("render - signal wait\n");
                mCondition.wait(mMutex);
                frameCount = 0;
                SXLOGD("render - signal rx'd\n");
            }

            if (mExit) break;

            // We could end up here if start() is called, and before we get a
            // chance to run, the app calls stop() or reset(). Re-check render
            // flag so we don't try to render in stop or reset state.
            if (!mRender) {
                continue;
           }

            if (!mAudioSink->ready()) {
                 SXLOGD("render - create output track\n");
                 if (createOutputTrack() != NO_ERROR)
                     break;
            }
        }

        // codec returns negative number on error
        if (numread < 0) {
            SXLOGE("Error in FMPlayer  numread=%ld",numread);
            sendEvent(MEDIA_ERROR);
            break;
        }

        // start audio output if necessary
        if (!audioStarted && !mPaused && !mExit) {
            SXLOGD("render - starting audio\n");
            mAudioSink->start();
            // setparameter to hardware after staring, for cr ALPS00073272
            AudioSystem::setParameters (0,DIGITAL_FM_ENABLE);
            audioStarted = true;
            firstOutput = true;

            //firstly push some amount of buffer to make the mixer alive
            if ((temp = mAudioSink->write(mDummyBuffer, bufSize)) < 0) {
               SXLOGE("Error in writing:%d",temp);
               result = temp;
               break;
            }
            if ((temp = mAudioSink->write(mDummyBuffer, bufSize)) < 0) {
               SXLOGE("Error in writing:%d",temp);
               result = temp;
               break;
            }
            if ((temp = mAudioSink->write(mDummyBuffer, bufSize)) < 0) {
               SXLOGE("Error in writing:%d",temp);
               result = temp;
               break;
            }

        }

        {
            Mutex::Autolock l(mMutex);
            int brt =0, art =0;
            //SXLOGD("[%lld] before read %d",brt=getTimeMs());
            if (firstOutput) {
               numread = I2SRead(mI2Sdriver, mI2Sid, mAudioBuffer, bufSize);
               firstOutput = false;
            }
            else {
            	 numread = I2SRead(mI2Sdriver, mI2Sid, mAudioBuffer, bufSize);
            }
            //SXLOGD("[%lld] after read %d",art=getTimeMs());
            if(art-brt > 90 )
               SXLOGW("read time abnormal");
            //SXLOGD("[%d]read %d bytes from I2S, id:%d",&mI2Sid, numread, mI2Sid);

            frameCount++;
        }

        lastTime = thisTime;
        thisTime = getTimeMs();
        if ( thisTime -lastTime > 160 )
           SXLOGW(" !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!time diff = %d", thisTime -lastTime);

        // Write data to the audio hardware
        dataCount+=numread;
        //SXLOGD("[%lld] FMAudioPlayer read data count: %d",getTimeMs(), dataCount);

       if ((temp = mAudioSink->write(mAudioBuffer, numread)) < 0) {
           SXLOGE("Error in writing:%d",temp);
           result = temp;
           break;
       }
       //SXLOGD("[%lld] after write writecount = %d" ,getTimeMs(),temp);
       //sleep to allow command to get mutex
       usleep(1000);
    }

threadExit:
    mAudioSink.clear();


    if (mAudioBuffer) {
        delete [] mAudioBuffer;
        mAudioBuffer = NULL;
    }
    if (mDummyBuffer) {
        delete [] mDummyBuffer;
        mDummyBuffer = NULL;
    }

    SXLOGD("[%d]render end mRenderTid=%d\n",mI2Sid, mRenderTid);

    // tell main thread goodbye
    Mutex::Autolock l(mMutex);
    mRenderTid = -1;
    mCondition.signal();

#ifdef FM_AUDIO_FILELOG
   fclose(fp);
#endif
    return result;
}

} // end namespace android

