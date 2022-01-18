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

static bool cmd_swipe_init(int argc, const char **argv, void **pw_out)
{
	int id;
	int speed = 800;
	enum {
		ARG_CDT,
		ARG_DISPLAY,
		ARG_SWIPE,
		ARG_X,
		ARG_Y,
		ARG_X_DIST,
		ARG_Y_DIST,
		ARG_SPEED,
		ARG__COUNT,
	};

	if (argc < ARG_SPEED || argc > ARG__COUNT) {
		fprintf(stderr, "Usage:\n");
		fprintf(stderr, "  %s %s %s <X> <Y> <X_DIST> <Y_DIST> [SPEED]\n",
				argv[ARG_CDT],
				argv[ARG_DISPLAY],
				argv[ARG_SWIPE]);
		fprintf(stderr, "\n");
		fprintf(stderr, "  X      -- X coordinate of start\n");
		fprintf(stderr, "  Y      -- Y coordinate of start\n");
		fprintf(stderr, "  X_DIST -- Scroll distance (X-axis, positive to scroll left)\n");
		fprintf(stderr, "  Y_DIST -- Scroll distance (Y-axis, positive to scroll left)\n");
		fprintf(stderr, "\n");
		fprintf(stderr, "Optional:\n");
		fprintf(stderr, "  SPEED  -- Default: 800\n");
		return false;
	}

	if (argc == ARG__COUNT) {
		speed = atoi(argv[ARG_SPEED]);
	}

	msg_queue_for_send(&(const struct msg)
		{
			.type = MSG_TYPE_SCROLL_GESTURE,
			.data = {
				.scroll_gesture = {
					.speed = speed,
					.x = atoi(argv[ARG_X]),
					.y = atoi(argv[ARG_Y]),
					.x_dist = atoi(argv[ARG_X_DIST]),
					.y_dist = atoi(argv[ARG_Y_DIST]),
				},
			},
		}, &id);

	*pw_out = NULL;
	return true;
}

static void cmd_swipe_msg(void *pw, int id, const char *msg, size_t len)
{
	(void)(pw);

	fprintf(stderr, "Received message with id %i: %*s\n",
			id, (int)len, msg);
}

const struct cmd_table cmd_swipe = {
	.cmd  = "swipe",
	.init = cmd_swipe_init,
	.msg  = cmd_swipe_msg,
};
