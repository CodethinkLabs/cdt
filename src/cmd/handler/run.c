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

static bool cmd_run_init(int argc, const char **argv, void **pw_out)
{
	int id;
	enum {
		ARG_CDT,
		ARG_DISPLAY,
		ARG_RUN,
		ARG_SCRIPT,
		ARG__COUNT,
	};

	if (argc != ARG__COUNT) {
		cmd_help(argc, argv, NULL);
		return false;
	}

	msg_queue_for_send(&(const struct msg)
		{
			.type = MSG_TYPE_EVALUATE,
			.data = {
				.evaluate = {
					.expression = argv[ARG_SCRIPT],
				},
			},
		}, &id);

	*pw_out = NULL;
	return true;
}

static void cmd_run_msg(void *pw, int id, const char *msg, size_t len)
{
	(void)(pw);

	fprintf(stderr, "Received message with id %i: %*s\n",
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
	enum {
		ARG_CDT,
		ARG_DISPLAY,
		ARG__COUNT,
	};

	CDT_UNUSED(argc);

	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "  %s %s %s <SCRIPT>\n",
			argv[ARG_CDT],
			argv[ARG_DISPLAY],
			cmd_run.cmd);
	fprintf(stderr, "\n");
	fprintf(stderr, "Parameters:\n");
	fprintf(stderr, "  SCRIPT     -- JSON-escaped JavaScript\n");
}
