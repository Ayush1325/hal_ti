/*
 * Copyright (c) 2013-2025, Texas Instruments Incorporated
 * Copyright (c) 2026 Ayush Singh <ayush@beagleboard.org>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * Entry point of the BLE5-Stack thread, created by ICall.
 *
 * Equivalent of startup_entry() in common/cc26xx/icall_startup.c for a stack
 * linked as a library (STACK_LIBRARY). That file only builds with the TI
 * and IAR compilers.
 */

#include <hal_types.h>
#include <icall.h>
#include <rom_jt.h>

extern int stack_main(void *arg);

void startup_entry(const ICall_RemoteTaskArg *arg0, void *arg1)
{
	ICall_dispatcher = arg0->dispatch;
	ICall_enterCriticalSection = arg0->entercs;
	ICall_leaveCriticalSection = arg0->leavecs;

#if defined(FLASH_ROM_BUILD)
	CommonROM_Init();
	ROM_Init();
#endif

	stack_main(arg1);
}
