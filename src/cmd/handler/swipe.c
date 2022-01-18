/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (c) 2022 Codethink
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "cmd/cmd.h"
#include "cmd/private.h"

#include "msg/msg.h"
#include "util/util.h"

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
		cmd_help(argc, argv, NULL);
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

static void cmd_swipe_help(int argc, const char **argv);

const struct cmd_table cmd_swipe = {
	.cmd  = "swipe",
	.init = cmd_swipe_init,
	.help = cmd_swipe_help,
	.msg  = cmd_swipe_msg,
};

static void cmd_swipe_help(int argc, const char **argv)
{
	enum {
		ARG_CDT,
		ARG_DISPLAY,
		ARG__COUNT,
	};

	CDT_UNUSED(argc);

	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "  %s %s %s <X> <Y> <X_DIST> <Y_DIST> [SPEED]\n",
			argv[ARG_CDT],
			argv[ARG_DISPLAY],
			cmd_swipe.cmd);
	fprintf(stderr, "\n");
	fprintf(stderr, "Parameters:\n");
	fprintf(stderr, "  X      -- X coordinate of start\n");
	fprintf(stderr, "  Y      -- Y coordinate of start\n");
	fprintf(stderr, "  X_DIST -- Scroll distance (X-axis)\n");
	fprintf(stderr, "  Y_DIST -- Scroll distance (Y-axis)\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Optional:\n");
	fprintf(stderr, "  SPEED  -- Pixels per second. (Default: 800)\n");
}
