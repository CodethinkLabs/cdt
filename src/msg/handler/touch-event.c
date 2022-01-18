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

#define PRINT_FMT_TOUCH_EVENT_START__ID_X_Y \
	"{" \
		"\"id\":%i," \
		"\"method\":\"Input.dispatchTouchEvent\"," \
		"\"params\":{" \
			"\"type\":\"touchStart\"," \
			"\"touchPoints\":[" \
				"{\"x\":%i,\"y\":%i}" \
			"]" \
		"}" \
	"}"

#define PRINT_FMT_TOUCH_EVENT_MOVE__ID_X_Y \
	"{" \
		"\"id\":%i," \
		"\"method\":\"Input.dispatchTouchEvent\"," \
		"\"params\":{" \
			"\"type\":\"touchMove\"," \
			"\"touchPoints\":[" \
				"{\"x\":%i,\"y\":%i}" \
			"]" \
		"}" \
	"}"

#define PRINT_FMT_TOUCH_EVENT_END__ID \
	"{" \
		"\"id\":%i," \
		"\"method\":\"Input.dispatchTouchEvent\"," \
		"\"params\":{" \
			"\"type\":\"touchEnd\"," \
			"\"touchPoints\":[]" \
		"}" \
	"}"

char *msg_str_touch_event(const struct msg *msg, int id)
{
	struct msg_container *m;

	switch (msg->type) {
	case MSG_TYPE_TOUCH_EVENT_END:
		if (!msg_create(&m,
				PRINT_FMT_TOUCH_EVENT_END__ID,
				id)) {
			return NULL;
		}
		break;

	case MSG_TYPE_TOUCH_EVENT_MOVE:
		if (!msg_create(&m,
				PRINT_FMT_TOUCH_EVENT_MOVE__ID_X_Y,
				id,
				msg->data.touch_event.x,
				msg->data.touch_event.y)) {
			return NULL;
		}
		break;

	case MSG_TYPE_TOUCH_EVENT_START:
		if (!msg_create(&m,
				PRINT_FMT_TOUCH_EVENT_START__ID_X_Y,
				id,
				msg->data.touch_event.x,
				msg->data.touch_event.y)) {
			return NULL;
		}
		break;

	default:
		fprintf(stderr, "%s: Unhandled message type: %i\n",
				__func__, msg->type);
		return NULL;
	}

	m->type = msg->type;
	m->id = id;

	return m->str;
}
