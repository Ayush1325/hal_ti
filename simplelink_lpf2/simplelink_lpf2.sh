#!/bin/sh

# A simple script to copy relevant files from TI Simplelink F2 SDK.

SOURCE=$1
OUTPUT=$2

mkdir -p ${OUTPUT}/source/ti
rsync --include='*/' --include='*.c' --include='*.h' --exclude='*' -amv ${SOURCE}/source/ti/drivers $OUTPUT/source/ti/

rsync --include='*/' --include='*.c' --include='*.h' --exclude='*' -amv ${SOURCE}/source/ti/log $OUTPUT/source/ti/

mkdir -p ${OUTPUT}/source/ti/devices
rsync --include='*/' --include='*.c' --include='*.h' --exclude='*' -amv ${SOURCE}/source/ti/devices/DeviceFamily.h $OUTPUT/source/ti/devices/
rsync --include='*/' --include='*.c' --include='*.h' --exclude='*' -amv ${SOURCE}/source/ti/devices/cc13x2x7_cc26x2x7 $OUTPUT/source/ti/devices/

# BLE5-Stack sources (the OneLib.a / ble_r2.symbols binaries are fetched as
# west blobs, see zephyr/module.yml)
mkdir -p ${OUTPUT}/source/ti/ble5stack ${OUTPUT}/source/ti/common
for dir in common config controller hal heapmgr host icall inc npi osal rom services; do
	rsync --include='*/' --include='*.c' --include='*.h' --include='*.opt' --exclude='*' -amv \
		${SOURCE}/source/ti/ble5stack/${dir} $OUTPUT/source/ti/ble5stack/
done
rsync --include='*/' --include='*.c' --include='*.h' --exclude='*' -amv ${SOURCE}/source/ti/common/nv $OUTPUT/source/ti/common/
mkdir -p ${OUTPUT}/source/ti/common/cc26xx
rsync --include='*/' --include='*.c' --include='*.h' --exclude='*' -amv ${SOURCE}/source/ti/common/cc26xx/ecc $OUTPUT/source/ti/common/cc26xx/

# Pre-generated SysConfig BLE radio settings for CC1352P7
mkdir -p ${OUTPUT}/source/ti/ble5stack/boards/cc1352p7
cp ${SOURCE}/source/ti/ti_sidewalk/apps/ti/sid_demo/app_source/cc1352p7/sysconfig/ble_ti_radio_config.c \
	$OUTPUT/source/ti/ble5stack/boards/cc1352p7/

echo "Please make Zephyr specific changes to source/ti/devices/cc13x2x7_cc26x2x7/startup_files/ccfg.c"
echo "Please make Zephyr specific changes to source/ti/drivers/power/PowerCC26X2.c"
