/*
 * Copyright (c) 2026 Ayush Singh <ayush@beagleboard.org>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * Zephyr implementation of the small OS interface (message queues, threads,
 * clock, heap) needed by the TI BLE5-Stack ICall POSIX port.
 *
 * This file is built with the default enum size. It only exchanges types
 * without enums with the BLE5-Stack library, which is built with
 * -fshort-enums.
 */

#include <errno.h>
#include <stdarg.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>

#include "mqueue.h"
#include "pthread.h"
#include "task.h"

#define TI_SHIM_MQ_COUNT    CONFIG_BT_TI_CC13XX_CC26XX_MQ_COUNT
#define TI_SHIM_MQ_MAX_MSGS 32
#define TI_SHIM_MQ_MSG_SIZE 8

struct ti_shim_mq {
	struct k_msgq msgq;
	bool nonblock;
	char buf[TI_SHIM_MQ_MAX_MSGS * TI_SHIM_MQ_MSG_SIZE] __aligned(4);
};

static struct ti_shim_mq ti_shim_mqs[TI_SHIM_MQ_COUNT];
static atomic_t ti_shim_mqs_used;

mqd_t ti_shim_mq_open(const char *name, int oflag, ...)
{
	const struct mq_attr *attr = NULL;
	struct ti_shim_mq *mq;
	size_t maxmsg = TI_SHIM_MQ_MAX_MSGS;
	size_t msgsize = sizeof(uint32_t);
	atomic_val_t idx;
	va_list ap;

	ARG_UNUSED(name);

	if ((oflag & O_CREAT) != 0) {
		va_start(ap, oflag);
		(void)va_arg(ap, int); /* mode */
		attr = va_arg(ap, const struct mq_attr *);
		va_end(ap);
	}

	if (attr != NULL) {
		maxmsg = attr->mq_maxmsg;
		msgsize = attr->mq_msgsize;
	}

	if ((maxmsg == 0U) || (maxmsg > TI_SHIM_MQ_MAX_MSGS) || (msgsize == 0U) ||
	    (msgsize > TI_SHIM_MQ_MSG_SIZE)) {
		__ASSERT(false, "unsupported mq_attr %zu x %zu", maxmsg, msgsize);
		return (mqd_t)-1;
	}

	idx = atomic_inc(&ti_shim_mqs_used);
	if (idx >= TI_SHIM_MQ_COUNT) {
		__ASSERT(false, "out of message queues, increase BT_TI_CC13XX_CC26XX_MQ_COUNT");
		return (mqd_t)-1;
	}

	mq = &ti_shim_mqs[idx];
	mq->nonblock = ((oflag & O_NONBLOCK) != 0);
	k_msgq_init(&mq->msgq, mq->buf, msgsize, maxmsg);

	return mq;
}

int ti_shim_mq_close(mqd_t mqdes)
{
	ARG_UNUSED(mqdes);

	return 0;
}

int ti_shim_mq_send(mqd_t mqdes, const char *msg_ptr, size_t msg_len, unsigned int msg_prio)
{
	k_timeout_t timeout = K_FOREVER;

	ARG_UNUSED(msg_prio);

	if (msg_len != mqdes->msgq.msg_size) {
		return -1;
	}

	if (mqdes->nonblock || k_is_in_isr()) {
		timeout = K_NO_WAIT;
	}

	return (k_msgq_put(&mqdes->msgq, msg_ptr, timeout) == 0) ? 0 : -1;
}

static ssize_t ti_shim_mq_get(mqd_t mqdes, char *msg_ptr, size_t msg_len, k_timeout_t timeout)
{
	if (msg_len < mqdes->msgq.msg_size) {
		return -1;
	}

	if (k_msgq_get(&mqdes->msgq, msg_ptr, timeout) != 0) {
		return -1;
	}

	return (ssize_t)mqdes->msgq.msg_size;
}

ssize_t ti_shim_mq_receive(mqd_t mqdes, char *msg_ptr, size_t msg_len, unsigned int *msg_prio)
{
	ARG_UNUSED(msg_prio);

	return ti_shim_mq_get(mqdes, msg_ptr, msg_len, mqdes->nonblock ? K_NO_WAIT : K_FOREVER);
}

ssize_t ti_shim_mq_timedreceive(mqd_t mqdes, char *msg_ptr, size_t msg_len,
				unsigned int *msg_prio, const struct timespec *abstime)
{
	int64_t now_ms = k_uptime_get();
	int64_t abs_ms = ((int64_t)abstime->tv_sec * MSEC_PER_SEC) +
			 (abstime->tv_nsec / NSEC_PER_MSEC);

	ARG_UNUSED(msg_prio);

	return ti_shim_mq_get(mqdes, msg_ptr, msg_len,
			      (abs_ms > now_ms) ? K_MSEC(abs_ms - now_ms) : K_NO_WAIT);
}

static K_THREAD_STACK_DEFINE(ti_shim_stack_thread_stack,
			     CONFIG_BT_TI_CC13XX_CC26XX_STACK_THREAD_STACK_SIZE);
static struct k_thread ti_shim_stack_thread;
static bool ti_shim_stack_thread_created;

static void ti_shim_thread_entry(void *p1, void *p2, void *p3)
{
	void *(*start_routine)(void *) = p1;

	ARG_UNUSED(p3);

	(void)start_routine(p2);
}

int ti_shim_pthread_attr_init(pthread_attr_t *attr)
{
	attr->priority = 0;
	attr->stacksize = 0;

	return 0;
}

int ti_shim_pthread_attr_setschedparam(pthread_attr_t *attr, const struct sched_param *param)
{
	attr->priority = param->sched_priority;

	return 0;
}

int ti_shim_pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize)
{
	attr->stacksize = stacksize;

	return 0;
}

/*
 * ICall only creates one remote thread: the BLE stack thread. Its stack and
 * priority come from Kconfig; the values requested by ICall are ignored.
 */
int ti_shim_pthread_create(pthread_t *thread, const pthread_attr_t *attr,
			   void *(*start_routine)(void *), void *arg)
{
	k_tid_t tid;

	if (ti_shim_stack_thread_created) {
		__ASSERT(false, "only one BLE stack thread is supported");
		return -1;
	}

	if ((attr != NULL) &&
	    (attr->stacksize > CONFIG_BT_TI_CC13XX_CC26XX_STACK_THREAD_STACK_SIZE)) {
		__ASSERT(false, "BLE stack thread needs %zu bytes of stack", attr->stacksize);
		return -1;
	}

	ti_shim_stack_thread_created = true;

	tid = k_thread_create(&ti_shim_stack_thread, ti_shim_stack_thread_stack,
			      K_THREAD_STACK_SIZEOF(ti_shim_stack_thread_stack),
			      ti_shim_thread_entry, start_routine, arg, NULL,
			      K_PRIO_COOP(CONFIG_BT_TI_CC13XX_CC26XX_STACK_THREAD_PRIO), 0,
			      K_NO_WAIT);
	k_thread_name_set(tid, "ti_ble_stack");

	*thread = tid;

	return 0;
}

pthread_t ti_shim_pthread_self(void)
{
	return k_current_get();
}

int ti_shim_pthread_getschedparam(pthread_t thread, int *policy, struct sched_param *param)
{
	if (policy != NULL) {
		*policy = 0;
	}

	param->sched_priority = k_thread_priority_get(thread);

	return 0;
}

int ti_shim_pthread_setschedparam(pthread_t thread, int policy, const struct sched_param *param)
{
	int prio = param->sched_priority;

	ARG_UNUSED(policy);

	if (prio == configMAX_PRIORITIES) {
		prio = K_HIGHEST_APPLICATION_THREAD_PRIO;
	}

	k_thread_priority_set(thread, prio);

	return 0;
}

int ti_shim_clock_gettime(int clock_id, struct timespec *tp)
{
	uint64_t ticks = k_uptime_ticks();

	ARG_UNUSED(clock_id);

	tp->tv_sec = (time_t)(ticks / CONFIG_SYS_CLOCK_TICKS_PER_SEC);
	tp->tv_nsec = (long)k_ticks_to_ns_floor64(ticks % CONFIG_SYS_CLOCK_TICKS_PER_SEC);

	return 0;
}

TaskHandle_t ti_shim_current_task(void)
{
	return k_current_get();
}

/*
 * ICall heap. The BLE stack allocates from interrupt context, so allocation
 * must never block.
 *
 * The stack relies on newly allocated memory being zero (for example the
 * link layer checks the RX data entry queue allocated in LL_Init() for a
 * NULL entry pointer), as it is on the TI heaps that start out zeroed.
 */
K_HEAP_DEFINE(ti_shim_heap, CONFIG_BT_TI_CC13XX_CC26XX_HEAP_SIZE);

void *ti_shim_heap_malloc(size_t size)
{
	void *ptr = k_heap_alloc(&ti_shim_heap, size, K_NO_WAIT);

	if (ptr != NULL) {
		memset(ptr, 0, size);
	}

	return ptr;
}

void ti_shim_heap_free(void *ptr)
{
	if (ptr != NULL) {
		k_heap_free(&ti_shim_heap, ptr);
	}
}

/* newlib extension used by the BLE5-Stack OSAL (_ltoa) */
char *ltoa(long value, char *buf, int radix)
{
	char tmp[33];
	unsigned long v;
	size_t i = 0;
	size_t j = 0;

	if ((radix < 2) || (radix > 36)) {
		buf[0] = '\0';
		return buf;
	}

	if ((radix == 10) && (value < 0)) {
		buf[j++] = '-';
		v = -(unsigned long)value;
	} else {
		v = (unsigned long)value;
	}

	do {
		uint8_t d = v % (unsigned long)radix;

		tmp[i++] = (d < 10U) ? ('0' + d) : ('a' + d - 10U);
		v /= (unsigned long)radix;
	} while (v != 0U);

	while (i > 0U) {
		buf[j++] = tmp[--i];
	}
	buf[j] = '\0';

	return buf;
}
