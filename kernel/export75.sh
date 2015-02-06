export ARCH=arm
export KERNEL_DIR=`pwd`

export PATH=/home/zormax/android/toolchain/arm-linux-androideabi-4.7/bin:$PATH
export CROSS_COMPILE=arm-linux-androideabi-
#export PATH=/home/zormax/android/toolchain/arm-cortex_a9-linux-gnueabihf-linaro_4.9.1-2014.06/bin:$PATH
#export CROSS_COMPILE=arm-eabi-

export PROJECT=kpt75kk
export TARGET_PRODUCT=kpt75kk
export MTK_ROOT_CUSTOM=../mediatek/custom/
export MTK_PATH_SOURCE=../mediatek/kernel/

export TARGET_BUILD_VARIANT=user

export ARCH_MTK_PLATFORM=mt6575
export MTK_PATH_PLATFORM=../mediatek/platform/mt6575/kernel/ 

