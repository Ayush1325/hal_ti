/*
 * Copyright (c) 2017, Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include <kernel/zephyr/dpl/dpl.h>
#include <ti/drivers/dpl/ClockP.h>

/*
 * ClockP ticks are 10 us, independent of the Zephyr system tick, as on the
 * TI-RTOS based SimpleLink platforms. TI code such as the BLE5-Stack ICall
 * layer relies on this tick period.
 */
#define CLOCKP_TICK_PERIOD 10U
#define CLOCKP_TICK_FREQ   (USEC_PER_SEC / CLOCKP_TICK_PERIOD)

static inline k_timeout_t ClockP_ticksToTimeout(uint32_t ticks)
{
	return K_USEC((uint64_t)ticks * CLOCKP_TICK_PERIOD);
}

/* 
 * ClockP_STRUCT_SIZE in ClockP.h must be updated to match the size of this
 * struct
 */
typedef struct _ClockP_Obj {
	struct k_timer timer;
	ClockP_Fxn clock_fxn;
	uintptr_t arg;
	uint32_t timeout; /* in ClockP ticks */
	uint32_t period; /* in ClockP ticks */
	bool active;
} ClockP_Obj;

static ClockP_Params ClockP_defaultParams = {
    .startFlag = false,
    .period = 0,
    .arg = 0,
};

static void expiry_fxn(struct k_timer *timer_id)
{
	ClockP_Obj *obj = (ClockP_Obj *)k_timer_user_data_get(timer_id);
	
	obj->clock_fxn(obj->arg);
}

/*
 *  ======== ClockP_construct ========
 *  @param timeout in ClockP ticks
 */
ClockP_Handle ClockP_construct(ClockP_Struct *handle, ClockP_Fxn clockFxn,
        uint32_t timeout, ClockP_Params *params)
{
	ClockP_Obj *obj = (ClockP_Obj *)handle;

	if (handle == NULL) {
		return NULL;
	}

	if (params == NULL) {
		params = &ClockP_defaultParams;
	}

	obj->clock_fxn = clockFxn;
	obj->arg = params->arg;
	obj->period = params->period;
	obj->timeout = timeout;
	obj->active = false;
	
	k_timer_init(&obj->timer, expiry_fxn, NULL);
	k_timer_user_data_set(&obj->timer, obj);
	
	if (params->startFlag) {
		ClockP_start(obj);
	}
	
	return  ((ClockP_Handle)handle);
}

/*
 *  ======== ClockP_getSystemTickFreq ========
 */
inline uint32_t ClockP_getSystemTickFreq()
{
   return CLOCKP_TICK_FREQ;
}

/*
 *  ======== ClockP_getSystemTickPeriod ========
 */
inline uint32_t ClockP_getSystemTickPeriod()
{
   return CLOCKP_TICK_PERIOD;
}

uint32_t ClockP_getSystemTicks()
{
	/* may wrap */
	return (uint32_t)(k_ticks_to_us_floor64(k_uptime_ticks()) / CLOCKP_TICK_PERIOD);
}

/*
 *  ======== ClockP_Params_init ========
 */
void ClockP_Params_init(ClockP_Params *params)
{
	params->arg = 0;
	params->startFlag = false;
	params->period = 0;
}

/*
 *  ======== ClockP_setTimeout ========
 *  @param timeout in ClockP ticks
 */
void ClockP_setTimeout(ClockP_Handle handle, uint32_t timeout)
{
	ClockP_Obj *obj = (ClockP_Obj *)handle;

	obj->timeout = timeout;
}

/*
 *  ======== ClockP_start ========
 */
void ClockP_start(ClockP_Handle handle)
{
	ClockP_Obj *obj = (ClockP_Obj *)handle;

	k_timer_start(&obj->timer, ClockP_ticksToTimeout(obj->timeout),
		      (obj->period != 0U) ? ClockP_ticksToTimeout(obj->period) : K_NO_WAIT);
	obj->active = true;
}

/*
 *  ======== ClockP_stop ========
 */
void ClockP_stop(ClockP_Handle handle)
{
	ClockP_Obj *obj = (ClockP_Obj *)handle;

	k_timer_stop(&obj->timer);
	obj->active = false;
}

/*
 *  ======== ClockP_usleep ========
 */
void ClockP_usleep(uint32_t usec)
{
	k_sleep(K_USEC(usec));
}

/*
 *  ======== ClockP_getTimeout ========
 */
uint32_t ClockP_getTimeout(ClockP_Handle handle) {
    ClockP_Obj *obj = (ClockP_Obj *)handle;
    if (!obj->active) {
        return obj->timeout;
    }

    return (uint32_t)(k_ticks_to_us_ceil64(k_timer_remaining_ticks(&obj->timer)) /
                      CLOCKP_TICK_PERIOD);
}

/*
 *  ======== ClockP_isActive ========
 */
bool ClockP_isActive(ClockP_Handle handle) {
    ClockP_Obj *obj = (ClockP_Obj *)handle;
    return obj->active;
}

void ClockP_destruct(ClockP_Struct *clockP)
{
	ClockP_Obj *obj = (ClockP_Obj *)clockP->data;

	obj->clock_fxn = NULL;
	obj->arg = 0;
	obj->period = 0;
	obj->timeout = 0;
	obj->active = false;

	k_timer_stop(&obj->timer);
}

#define DPL_MAX_CLOCKS 4 /* ICall timers of the BLE5-Stack */
K_MEM_SLAB_DEFINE(clock_slab, sizeof(ClockP_Obj), DPL_MAX_CLOCKS, MEM_ALIGN);

/*
 *  ======== ClockP_create ========
 */
ClockP_Handle ClockP_create(ClockP_Fxn clockFxn, uint32_t timeout, ClockP_Params *params)
{
	ClockP_Obj *obj;

	if (k_mem_slab_alloc(&clock_slab, (void **)&obj, K_NO_WAIT) != 0) {
		__ASSERT(false, "ClockP_create failed\r\n");
		return NULL;
	}

	return ClockP_construct((ClockP_Struct *)obj, clockFxn, timeout, params);
}

/*
 *  ======== ClockP_delete ========
 */
void ClockP_delete(ClockP_Handle handle)
{
	ClockP_Obj *obj = (ClockP_Obj *)handle;

	k_timer_stop(&obj->timer);
	k_mem_slab_free(&clock_slab, (void *)obj);
}
