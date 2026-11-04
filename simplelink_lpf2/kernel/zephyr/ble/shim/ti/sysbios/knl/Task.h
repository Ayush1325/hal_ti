/*
 * Copyright (c) 2026 Ayush Singh <ayush@beagleboard.org>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * icall_hci_tl.c includes the TI-RTOS task header for its thread statistics
 * report. Thread statistics are not available here, so the report contains
 * no threads.
 */

#ifndef TI_BLE_SHIM_SYSBIOS_TASK_H_
#define TI_BLE_SHIM_SYSBIOS_TASK_H_

#include <stddef.h>
#include <stdint.h>

typedef void *Task_Handle;

typedef struct {
	int priority;
	size_t used;
	size_t stackSize;
} Task_Stat;

static inline Task_Handle Task_Object_first(void)
{
	return NULL;
}

static inline Task_Handle Task_Object_next(Task_Handle handle)
{
	(void)handle;

	return NULL;
}

static inline void Task_stat(Task_Handle handle, Task_Stat *stat)
{
	(void)handle;
	(void)stat;
}

#endif /* TI_BLE_SHIM_SYSBIOS_TASK_H_ */
