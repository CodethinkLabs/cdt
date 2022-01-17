/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (c) 2022 Codethink
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "msg/msg.h"
#include "msg/queue.h"
#include "msg/private.h"

#define PRINT_FMT_START_SCREENCAST__ID_FMT \
	"{" \
		"\"id\":%i," \
		"\"method\":\"Page.startScreencast\"," \
		"\"params\":{" \
			"\"format\":\"%s\"," \
			"\"everyNthFrame\":1" \
		"}" \
	"}"

#define PRINT_FMT_START_SCREENCAST__ID_FMT_MAXW_MAXH \
	"{" \
		"\"id\":%i," \
		"\"method\":\"Page.startScreencast\"," \
		"\"params\":{" \
			"\"format\":\"%s\"," \
			"\"maxWidth\":%i," \
			"\"maxHeight\":%i," \
			"\"everyNthFrame\":1" \
		"}" \
	"}"

char *msg_str_start_screencast(const struct msg *msg, int id)
{
	struct msg_container *m;
	const char *fmt = msg->data.start_screencast.format;
	int h = msg->data.start_screencast.max_height;
	int w = msg->data.start_screencast.max_width;

	if (fmt == NULL) {
		fmt = "jpeg";
	}

	if (w == 0 || h == 0) {
		if (!msg_create(&m, PRINT_FMT_START_SCREENCAST__ID_FMT,
				id, fmt)) {
			return NULL;
		}
	} else {
		if (!msg_create(&m,
				PRINT_FMT_START_SCREENCAST__ID_FMT_MAXW_MAXH,
				id, fmt, w, h)) {
			return NULL;
		}
	}

	m->type = msg->type;
	m->id = id;

	return m->str;
}
