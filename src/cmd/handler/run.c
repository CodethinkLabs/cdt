/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (c) 2022 Codethink
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "cmd/cmd.h"
#include "cmd/private.h"

#include "msg/msg.h"

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

	if (argc < ARG__COUNT) {
		fprintf(stderr, "Usage:\n");
		fprintf(stderr, "  %s %s %s <SCRIPT>\n",
				argv[ARG_CDT],
				argv[ARG_DISPLAY],
				argv[ARG_RUN]);
		fprintf(stderr, "\n");
		fprintf(stderr, "SCRIPT     -- JSON-escaped JavaScript\n");
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

const struct cmd_table cmd_run = {
	.cmd  = "run",
	.init = cmd_run_init,
	.msg  = cmd_run_msg,
};
