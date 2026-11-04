/*
 * Copyright (c) 2026 Ayush Singh <ayush@beagleboard.org>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * The TI ICall POSIX port is written for FreeRTOS and uses a couple of
 * FreeRTOS definitions directly. Provide them on top of Zephyr.
 */

#ifndef TI_BLE_SHIM_FREERTOS_H_
#define TI_BLE_SHIM_FREERTOS_H_

/* Sentinel understood by ti_shim_pthread_setschedparam() */
#define configMAX_PRIORITIES 0x7fff

#endif /* TI_BLE_SHIM_FREERTOS_H_ */
