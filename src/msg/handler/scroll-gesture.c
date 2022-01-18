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

#define PRINT_FMT_SCROLL_GESTURE__ID_X_Y_SPEED_XDIST_YDIST \
	"{" \
		"\"id\":%i," \
		"\"method\":\"Input.synthesizeScrollGesture\"," \
		"\"params\":{" \
			"\"x\":%i," \
			"\"y\":%i," \
			"\"speed\":%i," \
			"\"xDistance\":%i," \
			"\"yDistance\":%i," \
			"\"preventFling\":false" \
		"}" \
	"}"

char *msg_str_scroll_gesture(const struct msg *msg, int id)
{
	struct msg_container *m;

	if (!msg_create(&m,
			PRINT_FMT_SCROLL_GESTURE__ID_X_Y_SPEED_XDIST_YDIST,
			id,
			msg->data.scroll_gesture.x,
			msg->data.scroll_gesture.y,
			msg->data.scroll_gesture.speed,
			msg->data.scroll_gesture.x_dist,
			msg->data.scroll_gesture.y_dist)) {
		return NULL;
	}

	m->type = msg->type;
	m->id = id;

	return m->str;
}
