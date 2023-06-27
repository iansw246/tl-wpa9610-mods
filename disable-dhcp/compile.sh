#!/bin/sh
# GPL Code kit
gcc_path=$HOME/src/tl-wpa9610/wpa9610_gpl_code/board/model_brcm_bcm47xx/toolchain/hndtools-arm-linux-2.6.36-uclibc-4.5.3/bin/arm-brcm-linux-uclibcgnueabi-gcc
include_path=$HOME/src/tl-wpa9610/wpa9610_gpl_code/board/model_brcm_bcm47xx/toolchain/hndtools-arm-linux-2.6.36-uclibc-4.5.3/arm-brcm-linux-uclibcgnueabi/sysroot/usr/include/
# Required so libraries that gcc depend on are able to be loaded
LD_LIBRARY_PATH="$HOME/src/tl-wpa9610/wpa9610_gpl_code/board/model_brcm_bcm47xx/toolchain/hndtools-arm-linux-2.6.36-uclibc-4.5.3/lib/:$LD_LIBRARY_PATH"
export LD_LIBRARY_PATH
# GPL Code kit
"${gcc_path}" -I"${include_path}" -Wall payload.c -o payload.out
#echo | ${gcc_path} -I"${include_path}" -E -Wp,-v -
