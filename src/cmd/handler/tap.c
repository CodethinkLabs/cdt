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

static struct tap_ctx {
	int64_t x;
	int64_t y;
	const char *display;
} tap_ctx;

static const struct cli_table_entry cli_entries[] = {
	{
		.p = true,
		.l = "tap",
		.t = CLI_CMD,
	},
	{
		.p = true,
		.l = "DISPLAY",
		.t = CLI_STRING,
		.v.s = &tap_ctx.display,
		.d = "Identifier for browser context to connect to."
	},
	{
		.p = true,
		.l = "X",
		.t = CLI_INT,
		.v.i = &tap_ctx.x,
		.d = "X coordinate to tap (px from top edge)."
	},
	{
		.p = true,
		.l = "Y",
		.t = CLI_INT,
		.v.i = &tap_ctx.y,
		.d = "Y coordinate to tap (px from left edge)."
	},
};
static const struct cli_table cli = {
	.entries = cli_entries,
	.count = (sizeof(cli_entries))/(sizeof(*cli_entries)),
	.min_positional = 4,
};

static bool cmd_tap_init(int argc, const char **argv,
		struct cmd_options *options, void **pw_out)
{
	int id;

	if (!cli_parse(&cli, argc, argv)) {
		cdt_log(CDT_LOG_ERROR, "Failed to parse command line");
		cmd_help(argc, argv, cli_entries[0].l);
		return false;
	}

	options->display = tap_ctx.display;

	msg_queue_for_send(&(const struct msg)
		{
			.type = MSG_TYPE_TOUCH_EVENT_START,
			.data = {
				.touch_event = {
					.x = (int)tap_ctx.x,
					.y = (int)tap_ctx.y,
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
	cli_help(&cli, (argc > 0) ? argv[0] : "cdt");
}
