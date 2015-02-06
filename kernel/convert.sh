#!/bin/bash 
curdir=`pwd`
mkimg="../mediatek/build/tools/mkimage"
kernel_img="${curdir}/arch/arm/boot/Image"
kernel_zimg="${curdir}/arch/arm/boot/zImage"

echo "**** Generate kernel.bin ****"

if [ ! -x ${mkimg} ]; then chmod a+x ${mkimg}; fi
${mkimg} ${kernel_zimg} KERNEL > kernel.bin
