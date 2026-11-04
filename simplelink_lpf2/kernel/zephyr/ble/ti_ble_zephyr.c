/*
 * Copyright (c) 2026 Ayush Singh <ayush@beagleboard.org>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * Runs the TI BLE5-Stack as an HCI controller for the Zephyr Bluetooth host.
 *
 * This replaces the NPI transport of the TI host_test example: HCI packets
 * from the Zephyr host are passed to HCI_TL_SendToStack(), and events and
 * ACL data from the stack are delivered to the Zephyr HCI driver through a
 * callback instead of a UART.
 *
 * Built as part of the BLE5-Stack library (-fshort-enums).
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <icall.h>
#include <icall_ble_api.h>
#include <icall_hci_tl.h>
#include <hal_assert.h>
#include <hci_tl.h>
#include <ble_user_config.h>
#include <ti_ble_config.h>
#include "inc/npi_ble.h"

#include <mqueue.h>
#include <ti/drivers/dpl/HwiP.h>

#include "ti_ble_zephyr.h"

#define TI_BLE_H4_CMD 0x01
#define TI_BLE_H4_ACL 0x02

#define TI_BLE_HCI_CMD_HDR_LEN 3U
#define TI_BLE_HCI_ACL_HDR_LEN 4U

/*
 * The host has at most one command and as many ACL packets as the controller
 * has buffers (MAX_NUM_PDU) outstanding.
 */
#define TI_BLE_TX_QUEUE_LEN (MAX_NUM_PDU + 3)
#define TI_BLE_CB_QUEUE_LEN 16
#define TI_BLE_HOST_QUEUE_LEN 4

#define TI_BLE_HCI_EVT_LE_META           0x3E
#define TI_BLE_HCI_OP_LE_SET_EVENT_MASK  0x2001
#define TI_BLE_LE_EVENT_MASK_LEN         8U

/*
 * LE event mask bits the controller needs on top of the ones enabled by the
 * host: icall_hci_tl.c implements the legacy scanning commands with
 * extended scanning, which only reports with LE Extended Advertising Report
 * (bit 12) enabled.
 */
static const uint8_t ti_ble_le_evt_mask_required[TI_BLE_LE_EVENT_MASK_LEN] = {
	0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

/* User configuration passed to the stack thread (see icall_addrs.h) */
icall_userCfg_t user0Cfg = BLE_USER_CFG;

struct ti_ble_cb_rec {
	void *data;
	void *fn;
};

static ti_ble_rx_cb_t ti_ble_rx_cb;
static ti_ble_assert_cb_t ti_ble_assert_cb;

static ICall_EntityID ti_ble_self_entity;
static ICall_SyncHandle ti_ble_sync;
static volatile bool ti_ble_app_ready;

static mqd_t ti_ble_tx_queue;
static mqd_t ti_ble_cb_queue;
static mqd_t ti_ble_host_queue;

/* LE event mask set by the host, default value from the Core Specification */
static uint8_t ti_ble_le_evt_mask[TI_BLE_LE_EVENT_MASK_LEN] = {
	0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

/*
 * Used by icall_hci_tl.c. Only provided by the TI-RTOS ICall port, where it
 * fails when the heap runs low; here it is the same as ICall_allocMsg().
 */
void *ICall_allocMsgLimited(size_t size)
{
	return ICall_allocMsg(size);
}

/*
 * Drop LE meta events the host did not enable (see ti_ble_le_evt_mask_required),
 * and TI specific LE meta events (subevent codes outside of the LE event mask,
 * such as the channel map update event 0x82).
 */
static bool ti_ble_le_evt_enabled(const uint8_t *pkt, uint16_t len)
{
	uint8_t bit;

	if ((len < 4U) || (pkt[0] != HCI_EVENT_PACKET) || (pkt[1] != TI_BLE_HCI_EVT_LE_META)) {
		return true;
	}

	if ((pkt[3] == 0U) || (pkt[3] > (TI_BLE_LE_EVENT_MASK_LEN * 8U))) {
		return false;
	}

	bit = pkt[3] - 1U;

	return (ti_ble_le_evt_mask[bit / 8U] & BV(bit % 8U)) != 0U;
}

static void ti_ble_deliver(const uint8_t *pkt, uint16_t len)
{
	if ((len > 0U) && (ti_ble_rx_cb != NULL) && ti_ble_le_evt_enabled(pkt, len)) {
		ti_ble_rx_cb(pkt, len);
	}
}

/*
 * icall_hci_tl.c also sends error reports from stack callbacks running in
 * interrupt context. Those are copied, prefixed with their length, and
 * delivered by the application thread.
 */
static void ti_ble_defer_to_host(const uint8_t *pkt, uint16_t len)
{
	uint8_t *buf = ICall_allocMsg(len + 2U);

	if (buf == NULL) {
		return;
	}

	buf[0] = LO_UINT16(len);
	buf[1] = HI_UINT16(len);
	memcpy(&buf[2], pkt, len);

	if (mq_send(ti_ble_host_queue, (const char *)&buf, sizeof(buf), 0) != 0) {
		ICall_freeMsg(buf);
		return;
	}

	ICall_signal(ti_ble_sync);
}

static void ti_ble_send_deferred(uint8_t *buf)
{
	ti_ble_deliver(&buf[2], BUILD_UINT16(buf[0], buf[1]));
	ICall_freeMsg(buf);
}

/*
 * Used by icall_hci_tl.c for events it generates itself (for example the
 * response to commands it emulates on top of the stack API).
 */
void NPITask_sendToHost(uint8_t *pMsg)
{
	npiPkt_t *pkt = (npiPkt_t *)pMsg;

	if (HwiP_inISR()) {
		ti_ble_defer_to_host(pkt->pData, pkt->pktLen);
	} else {
		ti_ble_deliver(pkt->pData, pkt->pktLen);
	}

	ICall_freeMsg(pMsg);
}

/* HCI_TL_CommandStatusCB: buf is a complete H:4 packet, freed by the caller */
static void ti_ble_send_to_host(uint8_t *buf, uint16_t len)
{
	if (HwiP_inISR()) {
		ti_ble_defer_to_host(buf, len);
	} else {
		ti_ble_deliver(buf, len);
	}
}

/*
 * HCI_TL_CallbackEvtProcessCB: icall_hci_tl.c defers work from stack
 * callbacks to the application thread through this function.
 */
static uint8_t ti_ble_post_callback(void *data, void *fn)
{
	struct ti_ble_cb_rec rec = {
		.data = data,
		.fn = fn,
	};

	if (mq_send(ti_ble_cb_queue, (const char *)&rec, sizeof(rec), 0) != 0) {
		return FALSE;
	}

	ICall_signal(ti_ble_sync);

	return TRUE;
}

static void ti_ble_assert_handler(uint8 cause, uint8 subcause)
{
	if (ti_ble_assert_cb != NULL) {
		ti_ble_assert_cb(cause, subcause);
	}
}

static void ti_ble_process_stack_msg(ICall_Hdr *msg)
{
	ICall_Stack_Event *evt = (ICall_Stack_Event *)msg;
	hciPacket_t *pkt = (hciPacket_t *)msg;
	uint16_t len;

	if (evt->signature == 0xFFFF) {
		/* OSAL event forwarded by ICall, nothing to do */
		return;
	}

	if (pkt->hdr.event != HCI_CTRL_TO_HOST_EVENT) {
		/* No TI host is built in, so only raw HCI packets are expected */
		return;
	}

	switch (pkt->pData[0]) {
	case HCI_EVENT_PACKET:
		len = HCI_EVENT_MIN_LENGTH + pkt->pData[2];
		ti_ble_deliver(pkt->pData, len);
		break;
	case HCI_ACL_DATA_PACKET:
		len = HCI_DATA_MIN_LENGTH + BUILD_UINT16(pkt->pData[3], pkt->pData[4]);
		ti_ble_deliver(pkt->pData, len);
		BM_free(pkt->pData);
		break;
	default:
		break;
	}
}

static void ti_ble_send_cmd(uint8_t *pkt, uint16_t len)
{
	hciPacket_t *msg;

	if ((len < (1U + TI_BLE_HCI_CMD_HDR_LEN)) ||
	    (len != (1U + TI_BLE_HCI_CMD_HDR_LEN + pkt[3]))) {
		return;
	}

	if ((BUILD_UINT16(pkt[1], pkt[2]) == TI_BLE_HCI_OP_LE_SET_EVENT_MASK) &&
	    (pkt[3] == TI_BLE_LE_EVENT_MASK_LEN)) {
		for (uint8_t i = 0U; i < TI_BLE_LE_EVENT_MASK_LEN; i++) {
			ti_ble_le_evt_mask[i] = pkt[4U + i];
			pkt[4U + i] |= ti_ble_le_evt_mask_required[i];
		}
	}

	msg = (hciPacket_t *)ICall_malloc(sizeof(hciPacket_t));
	if (msg == NULL) {
		return;
	}

	msg->hdr.event = HCI_HOST_TO_CTRL_CMD_EVENT;
	msg->hdr.status = 0xFF;
	msg->pData = pkt;

	HCI_TL_SendToStack((uint8_t *)msg);

	ICall_free(msg);
}

static void ti_ble_send_acl(uint8_t *pkt, uint16_t len)
{
	hciDataPacket_t *msg;
	uint16_t handle;
	uint16_t data_len;

	if (len < (1U + TI_BLE_HCI_ACL_HDR_LEN)) {
		return;
	}

	handle = BUILD_UINT16(pkt[1], pkt[2]);
	data_len = BUILD_UINT16(pkt[3], pkt[4]);
	if (len != (1U + TI_BLE_HCI_ACL_HDR_LEN + data_len)) {
		return;
	}

	msg = (hciDataPacket_t *)ICall_malloc(sizeof(hciDataPacket_t));
	if (msg == NULL) {
		return;
	}

	msg->hdr.event = HCI_HOST_TO_CTRL_DATA_EVENT;
	msg->hdr.status = 0xFF;
	msg->pktType = HCI_ACL_DATA_PACKET;
	msg->connHandle = handle & 0x0FFF;
	msg->pbFlag = (handle & 0x3000) >> 12;
	msg->pktLen = data_len;
	msg->pData = &pkt[1U + TI_BLE_HCI_ACL_HDR_LEN];

	HCI_TL_SendToStack((uint8_t *)msg);

	ICall_free(msg);
}

static void ti_ble_process_tx(uint8_t *pkt)
{
	uint16_t len = BUILD_UINT16(pkt[0], pkt[1]);
	uint8_t *h4 = &pkt[2];

	switch (h4[0]) {
	case TI_BLE_H4_CMD:
		ti_ble_send_cmd(h4, len);
		break;
	case TI_BLE_H4_ACL:
		ti_ble_send_acl(h4, len);
		break;
	default:
		break;
	}

	ICall_free(pkt);
}

int ti_ble_send(const uint8_t *pkt, uint16_t len)
{
	uint8_t *copy;

	/* 2 byte length prefix followed by the H:4 packet */
	copy = (uint8_t *)ICall_malloc(len + 2U);
	if (copy == NULL) {
		return -1;
	}

	copy[0] = LO_UINT16(len);
	copy[1] = HI_UINT16(len);
	memcpy(&copy[2], pkt, len);

	if (mq_send(ti_ble_tx_queue, (const char *)&copy, sizeof(copy), 0) != 0) {
		ICall_free(copy);
		return -1;
	}

	/* Packets queued before the application thread is up are sent once it starts */
	if (ti_ble_app_ready) {
		ICall_signal(ti_ble_sync);
	}

	return 0;
}

int ti_ble_init(ti_ble_rx_cb_t rx_cb, ti_ble_assert_cb_t assert_cb)
{
	struct mq_attr attr = {0};

	ti_ble_rx_cb = rx_cb;
	ti_ble_assert_cb = assert_cb;

	attr.mq_maxmsg = TI_BLE_TX_QUEUE_LEN;
	attr.mq_msgsize = sizeof(uint8_t *);
	ti_ble_tx_queue = mq_open("ti_ble_tx", O_CREAT | O_RDWR | O_NONBLOCK, 0, &attr);

	attr.mq_maxmsg = TI_BLE_CB_QUEUE_LEN;
	attr.mq_msgsize = sizeof(struct ti_ble_cb_rec);
	ti_ble_cb_queue = mq_open("ti_ble_cb", O_CREAT | O_RDWR | O_NONBLOCK, 0, &attr);

	attr.mq_maxmsg = TI_BLE_HOST_QUEUE_LEN;
	attr.mq_msgsize = sizeof(uint8_t *);
	ti_ble_host_queue = mq_open("ti_ble_host", O_CREAT | O_RDWR | O_NONBLOCK, 0, &attr);

	if ((ti_ble_tx_queue == (mqd_t)-1) || (ti_ble_cb_queue == (mqd_t)-1) ||
	    (ti_ble_host_queue == (mqd_t)-1)) {
		return -1;
	}

	RegisterAssertCback(ti_ble_assert_handler);

	/* Timer configuration of the ICall port, required by the stack */
	user0Cfg.appServiceInfo->timerTickPeriod = ICall_getTickPeriod();
	user0Cfg.appServiceInfo->timerMaxMillisecond = ICall_getMaxMSecs();

	ICall_init();
	ICall_createRemoteTasks();

	return 0;
}

void ti_ble_app_run(void)
{
	ICall_registerApp(&ti_ble_self_entity, &ti_ble_sync);

	HCI_TL_Init(NULL, ti_ble_send_to_host, ti_ble_post_callback, ti_ble_self_entity);
	HCI_TL_getCmdResponderID(
		ICall_getLocalMsgEntityId(ICALL_SERVICE_CLASS_BLE_MSG, ti_ble_self_entity));

	ti_ble_app_ready = true;

	for (;;) {
		ICall_ServiceEnum src;
		ICall_EntityID dest;
		ICall_Hdr *msg;
		struct ti_ble_cb_rec rec;
		uint8_t *pkt;

		/*
		 * Events come from the BLE service, ACL data from the BLE
		 * message (OSAL proxy) service; both are HCI packets for us.
		 */
		while (ICall_fetchServiceMsg(&src, &dest, (void **)&msg) == ICALL_ERRNO_SUCCESS) {
			ti_ble_process_stack_msg(msg);
			ICall_freeMsg(msg);
		}

		while (mq_receive(ti_ble_host_queue, (char *)&pkt, sizeof(pkt), NULL) ==
		       (ssize_t)sizeof(pkt)) {
			ti_ble_send_deferred(pkt);
		}

		while (mq_receive(ti_ble_cb_queue, (char *)&rec, sizeof(rec), NULL) ==
		       (ssize_t)sizeof(rec)) {
			((void (*)(void *))rec.fn)(rec.data);
		}

		while (mq_receive(ti_ble_tx_queue, (char *)&pkt, sizeof(pkt), NULL) ==
		       (ssize_t)sizeof(pkt)) {
			ti_ble_process_tx(pkt);
		}

		ICall_wait(ICALL_TIMEOUT_FOREVER);
	}
}
