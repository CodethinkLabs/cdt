/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (c) 2022 Codethink
 */

#ifndef CDT_UTIL_BUFFER_H
#define CDT_UTIL_BUFFER_H

struct cdt_buffer {
	char *data;
	size_t len;
	size_t alloc_size;
};

bool cdt_buffer_append(struct cdt_buffer *buf,
		const char *msg, size_t len);

void cdt_buffer_clear(struct cdt_buffer *buf);

void cdt_buffer_delete(struct cdt_buffer *buf);

#endif
