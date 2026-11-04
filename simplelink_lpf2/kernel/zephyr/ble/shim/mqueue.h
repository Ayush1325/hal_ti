/*
 * Copyright (c) 2026 Ayush Singh <ayush@beagleboard.org>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * Minimal message queue interface used by the TI ICall POSIX port
 * (icall_POSIX.c). Backed by Zephyr k_msgq, so that mq_send() can be used
 * from interrupt context, which the BLE stack does through ICall_signal().
 *
 * This header is only on the include path of the BLE5-Stack library. The
 * functions are renamed so they never clash with Zephyr's POSIX API.
 */

#ifndef TI_BLE_SHIM_MQUEUE_H_
#define TI_BLE_SHIM_MQUEUE_H_

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef O_RDONLY
#define O_RDONLY   0x0000
#endif
#ifndef O_WRONLY
#define O_WRONLY   0x0001
#endif
#ifndef O_RDWR
#define O_RDWR     0x0002
#endif
#ifndef O_CREAT
#define O_CREAT    0x0200
#endif
#ifndef O_NONBLOCK
#define O_NONBLOCK 0x4000
#endif

#define mqd_t   ti_shim_mqd_t
#define mq_attr ti_shim_mq_attr

struct ti_shim_mq;
typedef struct ti_shim_mq *mqd_t;

struct mq_attr {
	long mq_flags;
	long mq_maxmsg;
	long mq_msgsize;
	long mq_curmsgs;
};

/* TI code uses both "struct mq_attr" and plain "mq_attr" */
typedef struct mq_attr mq_attr;

#define mq_open         ti_shim_mq_open
#define mq_close        ti_shim_mq_close
#define mq_send         ti_shim_mq_send
#define mq_receive      ti_shim_mq_receive
#define mq_timedreceive ti_shim_mq_timedreceive

mqd_t ti_shim_mq_open(const char *name, int oflag, ...);
int ti_shim_mq_close(mqd_t mqdes);
int ti_shim_mq_send(mqd_t mqdes, const char *msg_ptr, size_t msg_len, unsigned int msg_prio);
ssize_t ti_shim_mq_receive(mqd_t mqdes, char *msg_ptr, size_t msg_len, unsigned int *msg_prio);
ssize_t ti_shim_mq_timedreceive(mqd_t mqdes, char *msg_ptr, size_t msg_len,
				unsigned int *msg_prio, const struct timespec *abstime);

#ifdef __cplusplus
}
#endif

#endif /* TI_BLE_SHIM_MQUEUE_H_ */
