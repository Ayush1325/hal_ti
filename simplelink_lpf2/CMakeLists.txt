if(CONFIG_HAS_CC13X2X7_CC26X2X7_SDK)
  if(CONFIG_SOC_CC1352R7 OR CONFIG_SOC_CC1352P7)
    zephyr_compile_definitions(DeviceFamily_CC13X2X7 ${COMPILER})
  elseif(CONFIG_SOC_CC2652R7 OR CONFIG_SOC_CC2652P7)
    zephyr_compile_definitions(DeviceFamily_CC26X2X7 ${COMPILER})
  endif()

  zephyr_include_directories(
	  . 
	  source 
	  source/ti/devices/cc13x2x7_cc26x2x7
  )

  zephyr_library()
  zephyr_library_sources(
	# device driverlib files
	source/ti/devices/cc13x2x7_cc26x2x7/driverlib/setup.c
	source/ti/devices/cc13x2x7_cc26x2x7/driverlib/chipinfo.c
	source/ti/devices/cc13x2x7_cc26x2x7/driverlib/aux_sysif.c
	source/ti/devices/cc13x2x7_cc26x2x7/driverlib/prcm.c
	source/ti/devices/cc13x2x7_cc26x2x7/driverlib/osc.c
	source/ti/devices/cc13x2x7_cc26x2x7/driverlib/cpu.c
	source/ti/devices/cc13x2x7_cc26x2x7/driverlib/sys_ctrl.c

	# PM
	source/ti/drivers/power/PowerCC26X2.c
	source/ti/drivers/power/PowerCC26X2_helpers.c
	source/ti/drivers/power/PowerCC26X2_calibrateRCOSC.c
	source/ti/drivers/power/PowerCC26X2_calibrateRCOSC_helpers.c
	source/ti/drivers/utils/List.c

	# DPL
	kernel/zephyr/dpl/ClockP_zephyr.c
	kernel/zephyr/dpl/HwiP_zephyr.c
	kernel/zephyr/dpl/SemaphoreP_zephyr.c
	kernel/zephyr/dpl/SwiP_zephyr.c
	kernel/zephyr/dpl/QueueP_zephyr.c
  )

  # Required for on-chip flash support
  zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_CC13XX_CC26XX source/ti/devices/cc13x2x7_cc26x2x7/driverlib/flash.c)

  # Required for IEEE 802.15.4 support
  zephyr_library_sources_ifdef(CONFIG_IEEE802154_CC13XX_CC26XX 
	  source/ti/devices/cc13x2x7_cc26x2x7/rf_patches/rf_patch_cpe_multi_protocol.c
	  source/ti/devices/cc13x2x7_cc26x2x7/rf_patches/rf_patch_cpe_ieee_802_15_4.c
	  source/ti/devices/cc13x2x7_cc26x2x7/driverlib/rfc.c
  )

  # Required for IEEE 802.15.4g support
  zephyr_library_sources_ifdef(CONFIG_IEEE802154_CC13XX_CC26XX_SUB_GHZ 
	  source/ti/devices/cc13x2x7_cc26x2x7/rf_patches/rf_patch_cpe_multi_protocol.c
	  source/ti/devices/cc13x2x7_cc26x2x7/driverlib/rfc.c
  )

  zephyr_library_sources_ifdef(CONFIG_ADC_CC13XX_CC26XX source/ti/devices/cc13x2x7_cc26x2x7/driverlib/aux_adc.c)

  if(CONFIG_BT_TI_CC13XX_CC26XX)
    zephyr_include_directories(kernel/zephyr/ble/include)
    if(CONFIG_BUILD_ONLY_NO_BLOBS)
      # The stack cannot be linked without its blobs, only provide its interface
      zephyr_library_sources(kernel/zephyr/ble/ti_ble_stubs.c)
    else()
      # Zephyr side of the BLE5-Stack OS shim, built with the default enum size
      zephyr_library_sources(kernel/zephyr/ble/shim/ti_ble_shim.c)
      include(ble5stack.cmake)
    endif()
  else()
    # With BLE, the RF driver is built as part of the BLE5-Stack library
    zephyr_library_sources(source/ti/drivers/rf/RFCC26X2_multiMode.c)
  endif()
endif()
