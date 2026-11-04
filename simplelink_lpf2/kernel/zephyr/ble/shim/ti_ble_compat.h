/*
 * Copyright (c) 2026 Ayush Singh <ayush@beagleboard.org>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * Definitions the BLE5-Stack sources expect from the FreeRTOS/newlib
 * environment they are written for. Included in every file of the
 * BLE5-Stack library.
 */

#ifndef TI_BLE_SHIM_COMPAT_H_
#define TI_BLE_SHIM_COMPAT_H_

#include <signal.h>
#include <time.h>

#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 1
#endif
#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 4
#endif

/* TI code uses the POSIX sigevent structure without the struct keyword */
typedef struct sigevent sigevent;

/* newlib extension used by osal.c */
char *ltoa(long value, char *buf, int radix);

#endif /* TI_BLE_SHIM_COMPAT_H_ */
