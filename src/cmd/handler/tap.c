/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (c) 2022 Codethink
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "cmd/cmd.h"
#include "cmd/private.h"

#include "msg/msg.h"

static bool cmd_tap_init(int argc, const char **argv, void **pw_out)
{
	int id;
	enum {
		ARG_CDT,
		ARG_DISPLAY,
		ARG_TAP,
		ARG_X,
		ARG_Y,
		ARG__COUNT,
	};

	if (argc < ARG__COUNT) {
		fprintf(stderr, "Usage:\n");
		fprintf(stderr, "  %s %s %s <X> <Y>\n",
				argv[ARG_CDT],
				argv[ARG_DISPLAY],
				argv[ARG_TAP]);
		return false;
	}

	msg_queue_for_send(&(const struct msg)
		{
			.type = MSG_TYPE_TOUCH_EVENT_START,
			.data = {
				.touch_event = {
					.x = atoi(argv[ARG_X]),
					.y = atoi(argv[ARG_Y]),
				},
			},
		}, &id);

	msg_queue_for_send(&(const struct msg)
		{
			.type = MSG_TYPE_TOUCH_EVENT_END,
		}, &id);

	*pw_out = NULL;
	return true;
}

static void cmd_tap_msg(void *pw, int id, const char *msg, size_t len)
{
	(void)(pw);

	fprintf(stderr, "Received message with id %i: %*s\n",
			id, (int)len, msg);
}

const struct cmd_table cmd_tap = {
	.cmd  = "tap",
	.init = cmd_tap_init,
	.msg  = cmd_tap_msg,
};
