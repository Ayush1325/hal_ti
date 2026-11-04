/*
 * Copyright (c) 2026 Ayush Singh <ayush@beagleboard.org>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * Declarations for the SysConfig generated BLE radio settings in
 * source/ti/ble5stack/boards/cc1352p7/ble_ti_radio_config.c
 * (RF design: LP_CC1352P7-1).
 */

#ifndef TI_RADIO_CONFIG_H_
#define TI_RADIO_CONFIG_H_

#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/rf_mailbox.h)
#include DeviceFamily_constructPath(driverlib/rf_common_cmd.h)
#include DeviceFamily_constructPath(driverlib/rf_ble_cmd.h)
#include <ti/drivers/rf/RF.h>

#define TXPOWERTABLE_2400_PA5_SIZE 16

extern RF_TxPowerTable_Entry txPowerTable_2400_pa5[];

#define RF_BLE_txPowerTable        txPowerTable_2400_pa5
#define RF_BLE_TX_POWER_TABLE_SIZE TXPOWERTABLE_2400_PA5_SIZE

extern RF_Mode RF_modeBle;

extern uint32_t pOverrides_bleCommon[];
extern uint32_t pOverrides_ble1Mbps[];
extern uint32_t pOverrides_ble2Mbps[];
extern uint32_t pOverrides_bleCoded[];

/* Index of BLE_STACK_OVERRIDES() in pOverrides_bleCommon[] */
#define BLE_STACK_OVERRIDES_OFFSET 17

#endif /* TI_RADIO_CONFIG_H_ */
