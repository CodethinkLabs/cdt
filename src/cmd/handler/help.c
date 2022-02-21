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

static const char *command = NULL;
static const struct cli_table_entry cli_entries[] = {
	{
		.p = true,
		.l = "help",
		.t = CLI_CMD,
	},
	{
		.p = true,
		.l = "COMMAND",
		.t = CLI_STRING,
		.v.s = &command,
		.d = "Command to get help for."
	},
};
static const struct cli_table cli = {
	.entries = cli_entries,
	.count = (sizeof(cli_entries))/(sizeof(*cli_entries)),
	.min_positional = 1,
};

static bool cmd_help_init(int argc, const char **argv,
		struct cmd_options *options, void **pw_out)
{
	CDT_UNUSED(options);

	if (!cli_parse(&cli, argc, argv)) {
		cdt_log(CDT_LOG_ERROR, "Failed to parse command line");
	}

	cmd_help(argc, argv, command);

	*pw_out = NULL;
	return false;
}

static void cmd_help_help(int argc, const char **argv);

const struct cmd_table cmd_help_table = {
	.cmd  = "help",
	.help = cmd_help_help,
	.init = cmd_help_init,
};

static void cmd_help_help(int argc, const char **argv)
{
	cli_help(&cli, (argc > 0) ? argv[0] : "cdt");

	fprintf(stderr, "\n");
	cmd_print_command_list();
	fprintf(stderr, "\n");
}
