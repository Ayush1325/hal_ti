# TI BLE5-Stack as an HCI controller for the Zephyr Bluetooth host.
#
# The prebuilt stack (OneLib.a) is built with -fshort-enums, so everything it
# calls directly (ICall, OSAL, HCI transport layer, RF and crypto drivers) is
# built with -fshort-enums in its own library. The Zephyr HCI driver talks to
# it only through kernel/zephyr/ble/include/ti_ble_zephyr.h.

set(TI_BLE5STACK_DIR ${CMAKE_CURRENT_LIST_DIR}/source/ti/ble5stack)
set(TI_DRIVERS_DIR ${CMAKE_CURRENT_LIST_DIR}/source/ti/drivers)
set(TI_DEVICE_DIR ${CMAKE_CURRENT_LIST_DIR}/source/ti/devices/cc13x2x7_cc26x2x7)
set(TI_BLE_ZEPHYR_DIR ${CMAKE_CURRENT_LIST_DIR}/kernel/zephyr/ble)
set(TI_BLE5STACK_BLOBS ${ZEPHYR_HAL_TI_MODULE_DIR}/zephyr/blobs/simplelink_lpf2/ble5stack/cc13x2r1)

zephyr_library_named(ti_ble5stack)

zephyr_library_sources(
  # Zephyr glue
  ${TI_BLE_ZEPHYR_DIR}/ti_ble_zephyr.c
  ${TI_BLE_ZEPHYR_DIR}/ti_ble_nv.c
  ${TI_BLE_ZEPHYR_DIR}/ti_ble_startup.c
  ${TI_BLE_ZEPHYR_DIR}/osal_icall_ble.c
  ${TI_BLE_ZEPHYR_DIR}/config/ti_drivers_config.c
  ${TI_BLE5STACK_DIR}/boards/cc1352p7/ble_ti_radio_config.c

  # ICall
  ${TI_BLE5STACK_DIR}/common/cc26xx/freertos/icall_POSIX.c
  ${TI_BLE5STACK_DIR}/common/cc26xx/onboard.c
  ${TI_BLE5STACK_DIR}/icall/app/ble_user_config.c
  ${TI_BLE5STACK_DIR}/icall/app/icall_api_lite.c
  ${TI_BLE5STACK_DIR}/icall/app/icall_hci_tl.c
  ${TI_BLE5STACK_DIR}/icall/src/icall_cc2650.c
  ${TI_BLE5STACK_DIR}/icall/src/icall_lite_translation.c
  ${TI_BLE5STACK_DIR}/icall/src/icall_user_config.c
  ${TI_BLE5STACK_DIR}/icall/stack/ble_dispatch_JT.c
  ${TI_BLE5STACK_DIR}/icall/stack/ble_dispatch_lite.c
  ${TI_BLE5STACK_DIR}/icall/stack/ble_user_config_stack.c

  # HAL
  ${TI_BLE5STACK_DIR}/hal/src/common/hal_assert.c
  ${TI_BLE5STACK_DIR}/hal/src/target/_common/hal_flash_wrapper.c
  ${TI_BLE5STACK_DIR}/hal/src/target/_common/hal_trng_wrapper.c

  # OSAL
  ${TI_BLE5STACK_DIR}/osal/src/common/osal.c
  ${TI_BLE5STACK_DIR}/osal/src/common/osal_bufmgr.c
  ${TI_BLE5STACK_DIR}/osal/src/common/osal_cbtimer.c
  ${TI_BLE5STACK_DIR}/osal/src/common/osal_clock.c
  ${TI_BLE5STACK_DIR}/osal/src/common/osal_list.c
  ${TI_BLE5STACK_DIR}/osal/src/common/osal_memory_icall.c
  ${TI_BLE5STACK_DIR}/osal/src/common/osal_pwrmgr.c
  ${TI_BLE5STACK_DIR}/osal/src/common/osal_timers.c
  ${TI_BLE5STACK_DIR}/osal/src/mcu/cc26xx/osal_snv_wrapper.c

  # BLE ROM
  ${TI_BLE5STACK_DIR}/rom/agama_r1/common_rom_init.c
  ${TI_BLE5STACK_DIR}/rom/agama_r1/rom_init.c

  # Drivers used by the stack
  ${TI_DRIVERS_DIR}/rf/RFCC26X2_multiMode.c
  ${TI_DRIVERS_DIR}/AESCCM.c
  ${TI_DRIVERS_DIR}/aesccm/AESCCMCC26XX.c
  ${TI_DRIVERS_DIR}/AESCTR.c
  ${TI_DRIVERS_DIR}/aesctr/AESCTRCC26XX.c
  ${TI_DRIVERS_DIR}/AESCTRDRBG.c
  ${TI_DRIVERS_DIR}/aesctrdrbg/AESCTRDRBGXX.c
  ${TI_DRIVERS_DIR}/AESECB.c
  ${TI_DRIVERS_DIR}/aesecb/AESECBCC26XX.c
  ${TI_DRIVERS_DIR}/ECDH.c
  ${TI_DRIVERS_DIR}/ecdh/ECDHCC26X2.c
  ${TI_DRIVERS_DIR}/TRNG.c
  ${TI_DRIVERS_DIR}/trng/TRNGCC26XX.c
  ${TI_DRIVERS_DIR}/cryptoutils/cryptokey/CryptoKey.c
  ${TI_DRIVERS_DIR}/cryptoutils/cryptokey/CryptoKeyPlaintext.c
  ${TI_DRIVERS_DIR}/cryptoutils/ecc/ECCParamsCC26X2.c
  ${TI_DRIVERS_DIR}/cryptoutils/sharedresources/CryptoResourceCC26XX.c
  ${TI_DRIVERS_DIR}/cryptoutils/sharedresources/PKAResourceCC26XX.c
  ${TI_DRIVERS_DIR}/cryptoutils/utils/CryptoUtils.c
  ${TI_DRIVERS_DIR}/utils/Random.c
  ${TI_DRIVERS_DIR}/utils/StructRingBuf.c

  # driverlib / RF patches
  ${TI_DEVICE_DIR}/driverlib/aes.c
  ${TI_DEVICE_DIR}/driverlib/pka.c
  ${TI_DEVICE_DIR}/driverlib/rfc.c
  ${TI_DEVICE_DIR}/rf_patches/rf_patch_cpe_multi_protocol.c
)

zephyr_library_include_directories(
  ${TI_BLE_ZEPHYR_DIR}/include
  ${TI_BLE_ZEPHYR_DIR}/config
  ${TI_BLE_ZEPHYR_DIR}/shim
  ${TI_BLE5STACK_DIR}
  ${TI_BLE5STACK_DIR}/inc
  ${TI_BLE5STACK_DIR}/rom
  ${TI_BLE5STACK_DIR}/common/cc26xx
  ${TI_BLE5STACK_DIR}/common/cc26xx/freertos
  ${TI_BLE5STACK_DIR}/common/cc26xx/npi/stack
  ${TI_BLE5STACK_DIR}/controller/cc26xx/inc
  ${TI_BLE5STACK_DIR}/hal/src/inc
  ${TI_BLE5STACK_DIR}/hal/src/target
  ${TI_BLE5STACK_DIR}/hal/src/target/_common
  ${TI_BLE5STACK_DIR}/heapmgr
  ${TI_BLE5STACK_DIR}/icall/inc
  ${TI_BLE5STACK_DIR}/icall/src
  ${TI_BLE5STACK_DIR}/icall/src/inc
  ${TI_BLE5STACK_DIR}/npi/src
  ${TI_BLE5STACK_DIR}/osal/src/inc
  ${TI_BLE5STACK_DIR}/services/src/saddr
  ${TI_BLE5STACK_DIR}/services/src/sdata
  ${CMAKE_CURRENT_LIST_DIR}/source/ti/common/nv
  ${CMAKE_CURRENT_LIST_DIR}/source/ti/common/cc26xx
  ${CMAKE_CURRENT_LIST_DIR}/source/ti
  ${TI_DEVICE_DIR}/rf_patches
)

# Build component definitions shared with the prebuilt library
foreach(opt_file build_components.opt factory_config.opt)
  file(STRINGS ${TI_BLE5STACK_DIR}/config/${opt_file} opt_lines REGEX "^-D")
  foreach(opt ${opt_lines})
    string(REGEX REPLACE "^-D" "" opt ${opt})
    zephyr_library_compile_definitions(${opt})
  endforeach()
endforeach()

zephyr_library_compile_definitions(
  # Device / ROM build (same as the TI LP_CC1352P7-1 examples)
  CC13XX
  CC13X2
  FLASH_ROM_BUILD
  # ICall-lite with the stack linked as a library
  USE_ICALL
  ICALL_EVENTS
  ICALL_JT
  ICALL_LITE
  ICALL_LITE_12_PARAMS
  ICALL_MAX_NUM_ENTITIES=6
  ICALL_MAX_NUM_TASKS=3
  ICALL_STACK0_ADDR
  STACK_LIBRARY
  OSAL_CBTIMER_NUM_TASKS=1
  ONE_BLE_LIB_SIZE_OPTIMIZATION
  NVOCMP_NWSAMEITEM=1
  # ICall POSIX port (see kernel/zephyr/ble/shim)
  FREERTOS
  # HCI controller only: no TI host, full HCI transport layer
  HCI_TL_FULL
  CTRL_CONFIG=ADV_NCONN_CFG+ADV_CONN_CFG+SCAN_CFG+INIT_CFG
  # Emulate legacy advertising/scanning commands with the extended ones
  LEGACY_CMD
  # Configuration headers in kernel/zephyr/ble/config replace SysConfig output
  SYSCFG
)

# The ICall heap must be usable from interrupt context
set_source_files_properties(${TI_BLE5STACK_DIR}/common/cc26xx/freertos/icall_POSIX.c
  PROPERTIES COMPILE_DEFINITIONS "malloc=ti_shim_heap_malloc;free=ti_shim_heap_free")

zephyr_library_compile_options(
  -fshort-enums
  -std=gnu17
  -include ${TI_BLE_ZEPHYR_DIR}/shim/ti_ble_compat.h
  -Wno-unused
  -Wno-unused-parameter
  -Wno-missing-field-initializers
  -Wno-sign-compare
  -Wno-implicit-fallthrough
  -Wno-pointer-sign
  -Wno-int-conversion
  -Wno-incompatible-pointer-types
  -Wno-discarded-qualifiers
  -Wno-enum-compare
  -Wno-switch
  -Wno-type-limits
  -Wno-format
  -Wno-array-bounds
  -Wno-stringop-overflow
)

# Prebuilt stack and the addresses of its ROM part. The library is not linked
# as a whole archive, so only the objects that are used get pulled in.
zephyr_link_libraries(
  ${TI_BLE5STACK_BLOBS}/OneLib.a
  -Wl,--just-symbols=${TI_BLE5STACK_BLOBS}/ble_r2.symbols
  -Wl,--no-enum-size-warning
)
