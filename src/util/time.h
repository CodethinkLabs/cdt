/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (c) 2022 Michael Drake <tlsa@netsurf-browser.org>
 */

#ifndef CDT_UTIL_TIME_H
#define CDT_UTIL_TIME_H

static inline int64_t time_diff_ms(
		const struct timespec *time_start,
		const struct timespec *time_check)
{
	return ((time_check->tv_sec  - time_start->tv_sec) * 1000 +
		(time_check->tv_nsec - time_start->tv_nsec) / 1000000);
}

#endif
