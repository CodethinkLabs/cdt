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

#include "util/cli.h"
#include "util/log.h"
#include "util/util.h"

static struct swipe_ctx {
	int64_t x;
	int64_t y;
	int64_t speed;
	int64_t x_dist;
	int64_t y_dist;
	const char *display;
} swipe_ctx = {
	.speed = 800,
};

static const struct cli_table_entry cli_entries[] = {
	{
		.p = true,
		.l = "swipe",
		.t = CLI_CMD,
	},
	{
		.p = true,
		.l = "DISPLAY",
		.t = CLI_STRING,
		.v.s = &swipe_ctx.display,
		.d = "Identifier for browser context to connect to."
	},
	{
		.p = true,
		.l = "X",
		.t = CLI_INT,
		.v.i = &swipe_ctx.x,
		.d = "X coordinate of start."
	},
	{
		.p = true,
		.l = "Y",
		.t = CLI_INT,
		.v.i = &swipe_ctx.y,
		.d = "Y coordinate of start."
	},
	{
		.p = true,
		.l = "X_DIST",
		.t = CLI_INT,
		.v.i = &swipe_ctx.x_dist,
		.d = "Scroll distance (X-axis)."
	},
	{
		.p = true,
		.l = "Y_DIST",
		.t = CLI_INT,
		.v.i = &swipe_ctx.y_dist,
		.d = "Scroll distance (Y-axis)."
	},
	{
		.s = 's',
		.l = "speed",
		.t = CLI_INT,
		.v.i = &swipe_ctx.speed,
		.d = "Pixels per second. (Default: 800)."
	},
};
static const struct cli_table cli = {
	.entries = cli_entries,
	.count = (sizeof(cli_entries))/(sizeof(*cli_entries)),
	.min_positional = 6,
};

static bool cmd_swipe_init(int argc, const char **argv,
		struct cmd_options *options, void **pw_out)
{
	int id;

	if (!cli_parse(&cli, argc, argv)) {
		cdt_log(CDT_LOG_ERROR, "Failed to parse command line");
		cmd_help(argc, argv, cli_entries[0].l);
		return false;
	}

	options->display = swipe_ctx.display;

	msg_queue_for_send(&(const struct msg)
		{
			.type = MSG_TYPE_SCROLL_GESTURE,
			.data = {
				.scroll_gesture = {
					.x = (int)swipe_ctx.x,
					.y = (int)swipe_ctx.y,
					.speed = (int)swipe_ctx.speed,
					.x_dist = (int)swipe_ctx.x_dist,
					.y_dist = (int)swipe_ctx.y_dist,
				},
			},
		}, &id);

	*pw_out = NULL;
	return true;
}

static void cmd_swipe_msg(void *pw, int id, const char *msg, size_t len)
{
	(void)(pw);

	cdt_log(CDT_LOG_NOTICE, "Received message with id %i: %*s",
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
	cli_help(&cli, (argc > 0) ? argv[0] : "cdt");
}
