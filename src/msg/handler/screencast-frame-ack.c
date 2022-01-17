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

#define PRINT_FMT_SCREENCAST_FRAME_ACK__ID_SESSIONID \
	"{" \
		"\"id\":%i," \
		"\"method\":\"Page.screencastFrameAck\"," \
		"\"params\":{" \
			"\"sessionId\":%i" \
		"}" \
	"}"

char *msg_str_screencast_frame_ack(const struct msg *msg, int id)
{
	struct msg_container *m;

	if (!msg_create(&m,
			PRINT_FMT_SCREENCAST_FRAME_ACK__ID_SESSIONID,
			id, msg->data.screencast_frame_ack.session_id)) {
		return NULL;
	}

	m->type = msg->type;
	m->id = id;

	return m->str;
}
