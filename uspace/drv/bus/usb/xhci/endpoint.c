/*
 * Copyright (c) 2017 Petr Manek
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup drvusbxhci
 * @{
 */
/** @file
 * @brief The host controller endpoint management.
 */

#include <usb/host/utils/malloc32.h>
#include <usb/host/endpoint.h>
#include <usb/descriptor.h>

#include <errno.h>

#include "bus.h"
#include "commands.h"
#include "endpoint.h"

int xhci_endpoint_init(xhci_endpoint_t *xhci_ep, xhci_bus_t *xhci_bus)
{
	assert(xhci_ep);
	assert(xhci_bus);

	bus_t *bus = &xhci_bus->base;
	endpoint_t *ep = &xhci_ep->base;

	endpoint_init(ep, bus);

	return xhci_trb_ring_init(&xhci_ep->ring);
}

void xhci_endpoint_fini(xhci_endpoint_t *xhci_ep)
{
	assert(xhci_ep);

	/* FIXME: Tear down TR's? */
	xhci_trb_ring_fini(&xhci_ep->ring);
}

/** See section 4.5.1 of the xHCI spec.
 */
uint8_t xhci_endpoint_dci(xhci_endpoint_t *ep)
{
	return (2 * ep->base.target.endpoint) +
		(ep->base.transfer_type == USB_TRANSFER_CONTROL
		 || ep->base.direction == USB_DIRECTION_IN);
}

/** Return an index to the endpoint array. The indices are assigned as follows:
 * 0	EP0 BOTH
 * 1	EP1 OUT
 * 2	EP1 IN
 *
 * For control endpoints >0, the IN endpoint index is used.
 *
 * The index returned must be usually offset by a number of contexts preceding
 * the endpoint contexts themselves.
 */
uint8_t xhci_endpoint_index(xhci_endpoint_t *ep)
{
	return xhci_endpoint_dci(ep) - 1;
}

static int xhci_endpoint_type(xhci_endpoint_t *ep)
{
	const bool in = ep->base.direction == USB_DIRECTION_IN;

	switch (ep->base.transfer_type) {
	case USB_TRANSFER_CONTROL:
		return EP_TYPE_CONTROL;

	case USB_TRANSFER_ISOCHRONOUS:
		return in ? EP_TYPE_ISOCH_IN
			  : EP_TYPE_ISOCH_OUT;

	case USB_TRANSFER_BULK:
		return in ? EP_TYPE_BULK_IN
			  : EP_TYPE_BULK_OUT;

	case USB_TRANSFER_INTERRUPT:
		return in ? EP_TYPE_INTERRUPT_IN
			  : EP_TYPE_INTERRUPT_OUT;
	}

	return EP_TYPE_INVALID;
}

static void setup_control_ep_ctx(xhci_endpoint_t *ep, xhci_ep_ctx_t *ctx,
	xhci_trb_ring_t *ring)
{
	// EP0 is configured elsewhere.
	assert(ep->base.target.endpoint > 0);

	XHCI_EP_TYPE_SET(*ctx, xhci_endpoint_type(ep));
	XHCI_EP_MAX_PACKET_SIZE_SET(*ctx, ep->base.max_packet_size);
	XHCI_EP_ERROR_COUNT_SET(*ctx, 3);
	XHCI_EP_TR_DPTR_SET(*ctx, ring->dequeue);
	XHCI_EP_DCS_SET(*ctx, 1);
}

static void setup_bulk_ep_ctx(xhci_endpoint_t *ep, xhci_ep_ctx_t *ctx,
	xhci_trb_ring_t *ring, usb_superspeed_endpoint_companion_descriptor_t *ss_desc)
{
	XHCI_EP_TYPE_SET(*ctx, xhci_endpoint_type(ep));
	XHCI_EP_MAX_PACKET_SIZE_SET(*ctx, ep->base.max_packet_size);
	XHCI_EP_MAX_BURST_SIZE_SET(*ctx,
	    xhci_device_get(ep->base.device)->usb3 ? ss_desc->max_burst : 0);
	XHCI_EP_ERROR_COUNT_SET(*ctx, 3);

	// FIXME: Get maxStreams and other things from ss_desc
	const uint8_t maxStreams = 0;
	if (maxStreams > 0) {
		// TODO: allocate and clear primary stream array
		// TODO: XHCI_EP_MAX_P_STREAMS_SET(ctx, psa_size);
		// TODO: XHCI_EP_TR_DPTR_SET(ctx, psa_start_phys_addr);
		// TODO: set HID
		// TODO: set LSA
	} else {
		XHCI_EP_MAX_P_STREAMS_SET(*ctx, 0);
		XHCI_EP_TR_DPTR_SET(*ctx, ring->dequeue);
		XHCI_EP_DCS_SET(*ctx, 1);
	}
}

static void setup_isoch_ep_ctx(xhci_endpoint_t *ep, xhci_ep_ctx_t *ctx,
	xhci_trb_ring_t *ring, usb_superspeed_endpoint_companion_descriptor_t *ss_desc)
{
	XHCI_EP_TYPE_SET(*ctx, xhci_endpoint_type(ep));
	XHCI_EP_MAX_PACKET_SIZE_SET(*ctx, ep->base.max_packet_size & 0x07FF);
	XHCI_EP_MAX_BURST_SIZE_SET(*ctx, ss_desc->max_burst);
	// FIXME: get Mult field from SS companion descriptor somehow
	XHCI_EP_MULT_SET(*ctx, 0);
	XHCI_EP_ERROR_COUNT_SET(*ctx, 0);
	XHCI_EP_TR_DPTR_SET(*ctx, ring->dequeue);
	XHCI_EP_DCS_SET(*ctx, 1);
	// TODO: max ESIT payload
}

static void setup_interrupt_ep_ctx(xhci_endpoint_t *ep, xhci_ep_ctx_t *ctx,
	xhci_trb_ring_t *ring, usb_superspeed_endpoint_companion_descriptor_t *ss_desc)
{
	XHCI_EP_TYPE_SET(*ctx, xhci_endpoint_type(ep));
	XHCI_EP_MAX_PACKET_SIZE_SET(*ctx, ep->base.max_packet_size & 0x07FF);
	XHCI_EP_MAX_BURST_SIZE_SET(*ctx, ss_desc->max_burst);
	XHCI_EP_MULT_SET(*ctx, 0);
	XHCI_EP_ERROR_COUNT_SET(*ctx, 3);
	XHCI_EP_TR_DPTR_SET(*ctx, ring->dequeue);
	XHCI_EP_DCS_SET(*ctx, 1);
	// TODO: max ESIT payload
}

int xhci_device_add_endpoint(xhci_device_t *dev, xhci_endpoint_t *ep)
{
	assert(dev);
	assert(ep);

	int err = ENOMEM;
	const usb_endpoint_t ep_num = ep->base.target.endpoint;

	assert(&dev->base == ep->base.device);
	assert(dev->base.address == ep->base.target.address);
	assert(!dev->endpoints[ep_num]);

	dev->endpoints[ep_num] = ep;
	++dev->active_endpoint_count;

	if (ep_num == 0)
		/* EP 0 is initialized while setting up the device,
		 * so we must not issue the command now. */
		return EOK;

	// FIXME: Retrieve this from somewhere, if applicable.
	usb_superspeed_endpoint_companion_descriptor_t ss_desc;
	memset(&ss_desc, 0, sizeof(ss_desc));

	// Prepare input context.
	xhci_input_ctx_t *ictx = malloc32(sizeof(xhci_input_ctx_t));
	const unsigned ep_idx = xhci_endpoint_index(ep);
	if (!ictx)
		goto err;

	memset(ictx, 0, sizeof(xhci_input_ctx_t));

	// Quoting sec. 4.6.6: A1, D0, D1 are down, A0 is up.
	XHCI_INPUT_CTRL_CTX_ADD_CLEAR(ictx->ctrl_ctx, 1);
	XHCI_INPUT_CTRL_CTX_DROP_CLEAR(ictx->ctrl_ctx, 0);
	XHCI_INPUT_CTRL_CTX_DROP_CLEAR(ictx->ctrl_ctx, 1);
	XHCI_INPUT_CTRL_CTX_ADD_SET(ictx->ctrl_ctx, 0);
	XHCI_INPUT_CTRL_CTX_ADD_SET(ictx->ctrl_ctx, ep_idx + 1); /* Preceded by slot ctx */

	xhci_trb_ring_t *ep_ring = &ep->ring;
	xhci_ep_ctx_t *ep_ctx = &ictx->endpoint_ctx[ep_idx];

	// TODO: Convert to table
	switch (ep->base.transfer_type) {
	case USB_TRANSFER_CONTROL:
		setup_control_ep_ctx(ep, ep_ctx, ep_ring);
		break;

	case USB_TRANSFER_BULK:
		setup_bulk_ep_ctx(ep, ep_ctx, ep_ring, &ss_desc);
		break;

	case USB_TRANSFER_ISOCHRONOUS:
		setup_isoch_ep_ctx(ep, ep_ctx, ep_ring, &ss_desc);
		break;

	case USB_TRANSFER_INTERRUPT:
		setup_interrupt_ep_ctx(ep, ep_ctx, ep_ring, &ss_desc);
		break;
	}

	// Issue configure endpoint command (sec 4.3.5).
	xhci_cmd_t cmd;
	xhci_cmd_init(&cmd);

	cmd.slot_id = dev->slot_id;
	xhci_send_configure_endpoint_command(dev->hc, &cmd, ictx);
	if ((err = xhci_cmd_wait(&cmd, XHCI_DEFAULT_TIMEOUT)) != EOK)
		goto err_ictx;

	xhci_cmd_fini(&cmd);

	free32(ictx);
	return EOK;

err_ictx:
	free32(ictx);
err:
	dev->endpoints[ep_idx] = NULL;
	dev->active_endpoint_count--;
	return err;
}

int xhci_device_remove_endpoint(xhci_device_t *dev, xhci_endpoint_t *ep)
{
	assert(&dev->base == ep->base.device);
	assert(dev->base.address == ep->base.target.address);
	assert(dev->endpoints[ep->base.target.endpoint]);

	// TODO: Issue configure endpoint command to drop this endpoint.

	dev->endpoints[ep->base.target.endpoint] = NULL;
	--dev->active_endpoint_count;
	return EOK;
}

xhci_endpoint_t * xhci_device_get_endpoint(xhci_device_t *dev, usb_endpoint_t ep)
{
	return dev->endpoints[ep];
}

int xhci_device_configure(xhci_device_t *dev, xhci_hc_t *hc)
{
	int err;

	// Prepare input context.
	xhci_input_ctx_t *ictx = malloc(sizeof(xhci_input_ctx_t));
	if (!ictx) {
		return ENOMEM;
	}

	memset(ictx, 0, sizeof(xhci_input_ctx_t));

	// Quoting sec. 4.6.6: A1, D0, D1 are down, A0 is up.
	XHCI_INPUT_CTRL_CTX_ADD_CLEAR(ictx->ctrl_ctx, 1);
	XHCI_INPUT_CTRL_CTX_DROP_CLEAR(ictx->ctrl_ctx, 0);
	XHCI_INPUT_CTRL_CTX_DROP_CLEAR(ictx->ctrl_ctx, 1);
	XHCI_INPUT_CTRL_CTX_ADD_SET(ictx->ctrl_ctx, 0);

	// TODO: Set slot context and other flags. (probably forgot a lot of 'em)

	// Issue configure endpoint command (sec 4.3.5).
	xhci_cmd_t cmd;
	xhci_cmd_init(&cmd);

	cmd.slot_id = dev->slot_id;
	xhci_send_configure_endpoint_command(hc, &cmd, ictx);
	if ((err = xhci_cmd_wait(&cmd, XHCI_DEFAULT_TIMEOUT)) != EOK)
		goto err_cmd;

	xhci_cmd_fini(&cmd);
	return EOK;

err_cmd:
	free(ictx);
	return err;
}

/**
 * @}
 */
