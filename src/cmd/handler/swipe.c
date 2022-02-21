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
	int64_t direction;
	int64_t dist;
} swipe_ctx = {
	.speed = 800,
};

enum cmd_swipe_direction {
	CMD_SWIPE_UP,
	CMD_SWIPE_LEFT,
	CMD_SWIPE_DOWN,
	CMD_SWIPE_RIGHT,
};

static const struct cli_str_val cmd_cli_direction[] = {
	{ .str = "up"   , .val = CMD_SWIPE_UP   , },
	{ .str = "left" , .val = CMD_SWIPE_LEFT , },
	{ .str = "down" , .val = CMD_SWIPE_DOWN , },
	{ .str = "right", .val = CMD_SWIPE_RIGHT, },
	{ .str = NULL, },
};

static const struct cli_table_entry cli_entries[] = {
	CMD_CLI_COMMON("swipe"),
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
		.l = "DIRECTION",
		.t = CLI_ENUM,
		.v.e.e = &swipe_ctx.direction, \
		.v.e.desc = cmd_cli_direction, \
		.d = "Swipe gesture direction (up, left, down, right). "
		     "Note that this is gesture direction. If the gesture "
		     "is for scrolling, the page will move with the gesture."
	},
	{
		.p = true,
		.l = "DIST",
		.t = CLI_INT,
		.v.i = &swipe_ctx.dist,
		.d = "Swipe gesture distance (pixels)."
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
	int x_dist = 0;
	int y_dist = 0;

	if (!cmd_cli_parse(argc, argv, &cli, options)) {
		return false;
	}

	switch (swipe_ctx.direction) {
	case CMD_SWIPE_UP:    y_dist = -(int)swipe_ctx.dist; break;
	case CMD_SWIPE_DOWN:  y_dist =  (int)swipe_ctx.dist; break;
	case CMD_SWIPE_LEFT:  x_dist = -(int)swipe_ctx.dist; break;
	case CMD_SWIPE_RIGHT: x_dist =  (int)swipe_ctx.dist; break;
	}

	msg_queue_for_send(&(const struct msg)
		{
			.type = MSG_TYPE_SCROLL_GESTURE,
			.data = {
				.scroll_gesture = {
					.x = (int)swipe_ctx.x,
					.y = (int)swipe_ctx.y,
					.speed = (int)swipe_ctx.speed,
					.x_dist = x_dist,
					.y_dist = y_dist,
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
