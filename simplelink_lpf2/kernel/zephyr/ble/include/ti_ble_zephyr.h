/*
 * Copyright (c) 2026 Ayush Singh <ayush@beagleboard.org>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * Interface between the Zephyr HCI driver and the TI BLE5-Stack, which runs
 * as an HCI controller (no TI host).
 *
 * The BLE5-Stack is built with -fshort-enums. This interface only uses
 * fixed-width integer types and pointers, so it can be used from code built
 * with the default enum size.
 *
 * HCI packets are passed in H:4 format: the first byte is the packet type
 * (0x01 command, 0x02 ACL data, 0x04 event), followed by the HCI packet.
 */

#ifndef TI_BLE_ZEPHYR_H_
#define TI_BLE_ZEPHYR_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Called from the BLE application thread for each HCI event or ACL data
 * packet from the controller. The buffer is only valid during the call.
 */
typedef void (*ti_ble_rx_cb_t)(const uint8_t *pkt, uint16_t len);

/**
 * Called when the BLE5-Stack raises an assert.
 */
typedef void (*ti_ble_assert_cb_t)(uint8_t cause, uint8_t subcause);

/**
 * Start the BLE5-Stack thread. Call once, before ti_ble_app_run().
 *
 * @return 0 on success, negative value on failure.
 */
int ti_ble_init(ti_ble_rx_cb_t rx_cb, ti_ble_assert_cb_t assert_cb);

/**
 * Body of the BLE application thread. Registers with ICall and forwards HCI
 * packets between the Zephyr host and the BLE5-Stack. Does not return.
 */
void ti_ble_app_run(void);

/**
 * Queue an H:4 HCI command or ACL data packet for the controller. The
 * packet is copied. May be called from any thread.
 *
 * @return 0 on success, negative value on failure.
 */
int ti_ble_send(const uint8_t *pkt, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* TI_BLE_ZEPHYR_H_ */
