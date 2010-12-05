/*
 * Copyright (c) 2010 Vojtech Horky
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

/** @addtogroup libusb usb
 * @{
 */
/** @file
 * @brief USB driver - standard USB requests (implementation).
 */
#include <usb/usbdrv.h>
#include <usb/devreq.h>
#include <errno.h>

/** Change address of connected device.
 *
 * @see usb_drv_reserve_default_address
 * @see usb_drv_release_default_address
 * @see usb_drv_request_address
 * @see usb_drv_release_address
 * @see usb_drv_bind_address
 *
 * @param phone Open phone to HC driver.
 * @param old_address Current address.
 * @param address Address to be set.
 * @return Error code.
 */
int usb_drv_req_set_address(int phone, usb_address_t old_address,
    usb_address_t new_address)
{
	/* Prepare the target. */
	usb_target_t target = {
		.address = old_address,
		.endpoint = 0
	};

	/* Prepare the setup packet. */
	usb_device_request_setup_packet_t setup_packet = {
		.request_type = 0,
		.request = USB_DEVREQ_SET_ADDRESS,
		.index = 0,
		.length = 0,
	};
	setup_packet.value = new_address;

	usb_handle_t handle;
	int rc;

	/* Start the control write transfer. */
	rc = usb_drv_async_control_write_setup(phone, target,
	    &setup_packet, sizeof(setup_packet), &handle);
	if (rc != EOK) {
		return rc;
	}
	rc = usb_drv_async_wait_for(handle);
	if (rc != EOK) {
		return rc;
	}

	/* Finish the control write transfer. */
	rc = usb_drv_async_control_write_status(phone, target, &handle);
	if (rc != EOK) {
		return rc;
	}
	rc = usb_drv_async_wait_for(handle);
	if (rc != EOK) {
		return rc;
	}

	return EOK;
}

/**
 * @}
 */
