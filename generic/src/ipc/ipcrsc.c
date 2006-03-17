/*
 * Copyright (C) 2006 Ondrej Palkovsky
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

/* IPC resources management
 *
 * The goal of this source code is to properly manage IPC resources
 * and allow straight and clean clean-up procedure upon task termination.
 *
 * The pattern of usage of the resources is:
 * - allocate empty phone slot, connect | deallocate slot
 * - disconnect connected phone (some messages might be on the fly)
 * - find phone in slot and send a message using phone
 * - answer message to phone
 * 
 * 
 */

#include <synch/spinlock.h>
#include <ipc/ipc.h>
#include <arch.h>
#include <proc/task.h>
#include <ipc/ipcrsc.h>
#include <debug.h>

/** Find call_t * in call table according to callid
 *
 * @return NULL on not found, otherwise pointer to call structure
 */
call_t * get_call(__native callid)
{
	/* TODO: Traverse list of dispatched calls and find one */
	/* TODO: locking of call, ripping it from dispatched calls etc.  */
	return (call_t *) callid;
}

/** Return pointer to phone identified by phoneid or NULL if non-existent */
phone_t * get_phone_and_lock(__native phoneid)
{
	phone_t *phone;

	if (phoneid >= IPC_MAX_PHONES)
		return NULL;

	phone = &TASK->phones[phoneid];
	spinlock_lock(&phone->lock);
	if (!phone->callee) {
		spinlock_unlock(&phone->lock);
		return NULL;
	}
	return phone;
}

/** Allocate new phone slot in current TASK structure */
int phone_alloc(void)
{
	int i;

	spinlock_lock(&TASK->lock);
	
	for (i=0; i < IPC_MAX_PHONES; i++) {
		if (!TASK->phones[i].busy) {
			TASK->phones[i].busy = 1;
			break;
		}
	}
	spinlock_unlock(&TASK->lock);

	if (i >= IPC_MAX_PHONES)
		return -1;
	return i;
}

/** Disconnect phone */
void phone_dealloc(int phoneid)
{
	spinlock_lock(&TASK->lock);

	ASSERT(TASK->phones[phoneid].busy);

	if (TASK->phones[phoneid].callee)
		ipc_phone_destroy(&TASK->phones[phoneid]);

	TASK->phones[phoneid].busy = 0;
	spinlock_unlock(&TASK->lock);
}

void phone_connect(int phoneid, answerbox_t *box)
{
	phone_t *phone = &TASK->phones[phoneid];
	
	ipc_phone_connect(phone, box);
}
