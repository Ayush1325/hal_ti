/*
 * Copyright (c) 2026 Ayush Singh <ayush@beagleboard.org>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * Minimal pthread interface used by the TI ICall POSIX port (icall_POSIX.c)
 * to create the BLE stack thread and to temporarily raise the priority of
 * the calling thread. Backed by Zephyr threads.
 *
 * sched_priority values returned by pthread_getschedparam() are Zephyr
 * priorities and are only meant to be passed back to
 * pthread_setschedparam(). Setting configMAX_PRIORITIES raises the thread to
 * the highest cooperative priority.
 */

#ifndef TI_BLE_SHIM_PTHREAD_H_
#define TI_BLE_SHIM_PTHREAD_H_

#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include "mqueue.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The C library may already define these types (sys/types.h is included
 * above), so rename them for the code that includes this header.
 */
#define pthread_t      ti_shim_pthread_t
#define pthread_attr_t ti_shim_pthread_attr_t
#define sched_param    ti_shim_sched_param

typedef void *pthread_t;

struct sched_param {
	int sched_priority;
};

typedef struct {
	int priority;
	size_t stacksize;
} pthread_attr_t;

#define pthread_attr_init         ti_shim_pthread_attr_init
#define pthread_attr_setschedparam ti_shim_pthread_attr_setschedparam
#define pthread_attr_setstacksize ti_shim_pthread_attr_setstacksize
#define pthread_create            ti_shim_pthread_create
#define pthread_self              ti_shim_pthread_self
#define pthread_getschedparam     ti_shim_pthread_getschedparam
#define pthread_setschedparam     ti_shim_pthread_setschedparam

int ti_shim_pthread_attr_init(pthread_attr_t *attr);
int ti_shim_pthread_attr_setschedparam(pthread_attr_t *attr, const struct sched_param *param);
int ti_shim_pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize);
int ti_shim_pthread_create(pthread_t *thread, const pthread_attr_t *attr,
			   void *(*start_routine)(void *), void *arg);
pthread_t ti_shim_pthread_self(void);
int ti_shim_pthread_getschedparam(pthread_t thread, int *policy, struct sched_param *param);
int ti_shim_pthread_setschedparam(pthread_t thread, int policy, const struct sched_param *param);

/* Only CLOCK_MONOTONIC and CLOCK_REALTIME are used; both map to uptime. */
#define clock_gettime ti_shim_clock_gettime
int ti_shim_clock_gettime(int clock_id, struct timespec *tp);

#ifdef __cplusplus
}
#endif

#endif /* TI_BLE_SHIM_PTHREAD_H_ */
