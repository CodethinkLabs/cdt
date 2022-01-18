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

static bool cmd_help_init(int argc, const char **argv, void **pw_out)
{
	enum {
		ARG_CDT,
		ARG_DISPLAY,
		ARG_HELP,
		ARG_CMD,
		ARG__COUNT,
	};

	if (argc < ARG_CMD) {
		cmd_help(argc, argv, NULL);
	} else {
		cmd_help(argc, argv, argv[ARG_CMD]);
	}

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
	enum {
		ARG_CDT,
		ARG_DISPLAY,
		ARG__COUNT,
	};

	CDT_UNUSED(argc);

	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "  %s %s %s [CMD]\n",
			argv[ARG_CDT],
			argv[ARG_DISPLAY],
			cmd_help_table.cmd);
	fprintf(stderr, "\n");
	fprintf(stderr, "Optional:\n");
	fprintf(stderr, "  CMD -- Command to print specific help for.\n");
}
