/*
 * Copyright (c) 2026 Ayush Singh <ayush@beagleboard.org>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * Non-volatile storage interface for the BLE5-Stack.
 *
 * In this configuration the stack is only an HCI controller and the Zephyr
 * host stores bonding information, so the stack has nothing to persist.
 * Reads report that no item exists and writes are discarded.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <nvintf.h>
#include <nvocmp.h>

static uint8_t ti_ble_nv_init(void *param)
{
	(void)param;

	return NVINTF_SUCCESS;
}

static uint8_t ti_ble_nv_compact(uint16_t min_bytes)
{
	(void)min_bytes;

	return NVINTF_SUCCESS;
}

static uint8_t ti_ble_nv_read(NVINTF_itemID_t id, uint16_t offset, uint16_t length, void *buffer)
{
	(void)id;
	(void)offset;
	(void)length;
	(void)buffer;

	return NVINTF_NOTFOUND;
}

static uint8_t ti_ble_nv_write(NVINTF_itemID_t id, uint16_t length, void *buffer)
{
	(void)id;
	(void)length;
	(void)buffer;

	return NVINTF_SUCCESS;
}

static uint8_t ti_ble_nv_delete(NVINTF_itemID_t id)
{
	(void)id;

	return NVINTF_SUCCESS;
}

static uint32_t ti_ble_nv_get_item_len(NVINTF_itemID_t id)
{
	(void)id;

	return 0;
}

void NVOCMP_loadApiPtrsMin(NVINTF_nvFuncts_t *pfn)
{
	*pfn = (NVINTF_nvFuncts_t){
		.initNV = ti_ble_nv_init,
		.compactNV = ti_ble_nv_compact,
		.readItem = ti_ble_nv_read,
		.writeItem = ti_ble_nv_write,
		.deleteItem = ti_ble_nv_delete,
		.getItemLen = ti_ble_nv_get_item_len,
	};
}
