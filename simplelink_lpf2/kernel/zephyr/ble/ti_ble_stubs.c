/*
 * Copyright (c) 2026 Ayush Singh <ayush@beagleboard.org>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * BLE5-Stack interface for builds without the prebuilt stack
 * (CONFIG_BUILD_ONLY_NO_BLOBS). Allows building the Zephyr HCI driver, the
 * result does not work.
 */

#include <errno.h>

#include <zephyr/kernel.h>

#include "ti_ble_zephyr.h"

int ti_ble_init(ti_ble_rx_cb_t rx_cb, ti_ble_assert_cb_t assert_cb)
{
	ARG_UNUSED(rx_cb);
	ARG_UNUSED(assert_cb);

	return -ENOTSUP;
}

void ti_ble_app_run(void)
{
	k_sleep(K_FOREVER);
}

int ti_ble_send(const uint8_t *pkt, uint16_t len)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(len);

	return -ENOTSUP;
}
