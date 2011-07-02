/*
 * Copyright (c) 2011 Petr Koupy
 * Copyright (c) 2011 Jiri Zarevucky
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

/** @addtogroup libposix
 * @{
 */
/** @file
 */

#define LIBPOSIX_INTERNAL

#include "internal/common.h"
#include "time.h"

#include "libc/malloc.h"
#include "libc/task.h"
#include "libc/stats.h"

/**
 *
 * @param timep
 * @return
 */
struct posix_tm *posix_localtime(const time_t *timep)
{
	// TODO
	static struct posix_tm result = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	return &result;
}

/**
 *
 * @param tm
 * @return
 */
char *posix_asctime(const struct posix_tm *tm)
{
	// TODO
	static char result[] = "Sun Jan 01 00:00:00 1900\n";
	return result;
}

/**
 * 
 * @param timep
 * @return
 */
char *posix_ctime(const time_t *timep)
{
	return posix_asctime(posix_localtime(timep));
}

/**
 * 
 * @param s
 * @param maxsize
 * @param format
 * @param tm
 * @return
 */
size_t posix_strftime(char *s, size_t maxsize, const char *format, const struct posix_tm *tm)
{
	// TODO
	if (maxsize >= 1) {
		*s = '\0';
	}
	return 0;
}

/**
 * Get CPU time used since the process invocation.
 *
 * @return Consumed CPU cycles by this process or -1 if not available.
 */
posix_clock_t posix_clock(void)
{
	posix_clock_t total_cycles = -1;
	stats_task_t *task_stats = stats_get_task(task_get_id());
	if (task_stats) {
		total_cycles = (posix_clock_t) (task_stats->kcycles + task_stats->ucycles);
	}
	free(task_stats);
	task_stats = 0;

	return total_cycles;
}

/** @}
 */
