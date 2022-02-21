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

static struct run_ctx {
	const char *script;
} run_ctx;

static const struct cli_table_entry cli_entries[] = {
	CMD_CLI_COMMON("run"),
	{
		.p = true,
		.l = "SCRIPT",
		.t = CLI_STRING,
		.v.s = &run_ctx.script,
		.d = "JSON-escaped JavaScript."
	},
};
static const struct cli_table cli = {
	.entries = cli_entries,
	.count = (sizeof(cli_entries))/(sizeof(*cli_entries)),
	.min_positional = 3,
};

static bool cmd_run_init(int argc, const char **argv,
		struct cmd_options *options, void **pw_out)
{
	int id;

	if (!cmd_cli_parse(argc, argv, &cli, options)) {
		return false;
	}

	msg_queue_for_send(&(const struct msg)
		{
			.type = MSG_TYPE_EVALUATE,
			.data = {
				.evaluate = {
					.expression = run_ctx.script,
				},
			},
		}, &id);

	*pw_out = NULL;
	return true;
}

static void cmd_run_msg(void *pw, int id, const char *msg, size_t len)
{
	(void)(pw);

	cdt_log(CDT_LOG_NOTICE, "Received message with id %i: %*s",
			id, (int)len, msg);
}

static void cmd_run_help(int argc, const char **argv);

const struct cmd_table cmd_run = {
	.cmd  = "run",
	.init = cmd_run_init,
	.help = cmd_run_help,
	.msg  = cmd_run_msg,
};

static void cmd_run_help(int argc, const char **argv)
{
	cli_help(&cli, (argc > 0) ? argv[0] : "cdt");
}
