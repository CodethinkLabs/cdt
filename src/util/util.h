/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (c) 2022 Codethink
 */

#ifndef CDT_UTIL_UTIL_H
#define CDT_UTIL_UTIL_H

#define CDT_UNUSED(_u) ((void)(_u))

#define CDT_ARRAY_COUNT(_a) ((sizeof(_a)) / (sizeof(*_a)))

static inline const char *str_get_leaf(const char *str)
{
	const char *slash;

	slash = strrchr(str, '/');
	if (slash != NULL) {
		return slash + 1;
	}

	return str;
}

#endif
