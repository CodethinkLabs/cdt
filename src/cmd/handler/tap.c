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
} tap_ctx;

static const struct cli_table_entry cli_entries[] = {
	CMD_CLI_COMMON("tap"),
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

	if (!cmd_cli_parse(argc, argv, &cli, options)) {
		return false;
	}

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
