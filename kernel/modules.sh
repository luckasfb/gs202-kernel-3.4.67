#!/bin/bash 

#make modules
rm -rf mod
mkdir mod
cp `find ./ | grep .ko$` modules.builtin mod/
cp `find ../mediatek/ | grep .ko$` modules.builtin mod/
cd mod
#/home/zormax/android/toolchain/arm-cortex_a9-linux-gnueabihf-linaro_4.9.1-2014.06/bin/arm-eabi-strip --strip-debug ./*.ko
/home/zormax/android/toolchain/arm-linux-androideabi-4.7/bin/arm-linux-androideabi-strip --strip-debug ./*.ko