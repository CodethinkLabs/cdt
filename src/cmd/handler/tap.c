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

#include "util/log.h"
#include "util/util.h"

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
		cmd_help(argc, argv, NULL);
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

	cdt_log(CDT_LOG_NOTICE, "Received message with id %i: %*s",
			id, (int)len, msg);
}

static void cmd_tap_help(int argc, const char **argv);

const struct cmd_table cmd_tap = {
	.cmd  = "tap",
	.init = cmd_tap_init,
	.help = cmd_tap_help,
	.msg  = cmd_tap_msg,
};

static void cmd_tap_help(int argc, const char **argv)
{
	enum {
		ARG_CDT,
		ARG_DISPLAY,
		ARG__COUNT,
	};

	CDT_UNUSED(argc);

	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "  %s %s %s <X> <Y>\n",
			argv[ARG_CDT],
			argv[ARG_DISPLAY],
			cmd_tap.cmd);
	fprintf(stderr, "\n");
	fprintf(stderr, "Parameters:\n");
	fprintf(stderr, "  X -- X coordinate to tap (px from top edge).\n");
	fprintf(stderr, "  Y -- Y coordinate to tap (px from left edge).\n");
}
