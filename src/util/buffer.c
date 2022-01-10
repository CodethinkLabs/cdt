/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (c) 2022 Codethink
 */

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "util/buffer.h"

bool cdt_buffer_append(struct cdt_buffer *buf,
		const char *msg, size_t len)
{
	if (buf->alloc_size - buf->len < len + 1) {
		size_t new_size = buf->alloc_size * 2 + len + 1;
		char *tmp = realloc(buf->data, new_size);
		if (tmp == NULL) {
			return false;
		}
		buf->data = tmp;
		buf->alloc_size = new_size;
	}

	memcpy(buf->data + buf->len, msg, len);
	buf->len += len;
	buf->data[buf->len] = '\0';
	return true;
}

void cdt_buffer_clear(struct cdt_buffer *buf)
{
	buf->len = 0;

	if (buf->data != NULL) {
		buf->data[buf->len] = '\0';
	} else {
		buf->alloc_size = 0;
	}
}

void cdt_buffer_delete(struct cdt_buffer *buf)
{
	buf->len = 0;
	buf->alloc_size = 0;

	free(buf->data);
}
