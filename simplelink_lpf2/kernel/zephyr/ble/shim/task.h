/*
 * Copyright (c) 2026 Ayush Singh <ayush@beagleboard.org>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TI_BLE_SHIM_TASK_H_
#define TI_BLE_SHIM_TASK_H_

#include "FreeRTOS.h"

typedef void *TaskHandle_t;

#define xTaskGetCurrentTaskHandle ti_shim_current_task
TaskHandle_t ti_shim_current_task(void);

#endif /* TI_BLE_SHIM_TASK_H_ */
