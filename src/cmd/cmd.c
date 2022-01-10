/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (c) 2022 Codethink
 */

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

#include "cmd/cmd.h"
#include "cmd/private.h"

#include "util/util.h"

static struct {
	const struct cmd_table *cmd;
} cmd_g;

extern const struct cmd_table cmd_tap;

const struct cmd_table *cmd_table[] = {
	&cmd_tap,
};

bool cmd_init(int argc, const char **argv, void **pw_out)
{
	enum {
		ARG_CDT,
		ARG_DISPLAY,
		ARG_CMD,
		ARG__COUNT,
	};

	if (argc < ARG__COUNT) {
		fprintf(stderr, "Usage:\n");
		fprintf(stderr, "  %s %s <CMD>\n",
				argv[ARG_CDT], argv[ARG_DISPLAY]);
		fprintf(stderr, "\n");
		fprintf(stderr, "Commands:\n");
		for (size_t i = 0; i < CDT_ARRAY_COUNT(cmd_table); i++) {
			if (cmd_table[i] != NULL) {
				fprintf(stderr, "- %s\n", cmd_table[i]->cmd);
			}
		}
		fprintf(stderr, "\n");
		return false;
	}

	for (size_t i = 0; i < CDT_ARRAY_COUNT(cmd_table); i++) {
		if (cmd_table[i] != NULL) {
			if (strcmp(argv[ARG_CMD], cmd_table[i]->cmd) == 0) {
				cmd_g.cmd = cmd_table[i];
				if (cmd_table[i]->init != NULL) {
					return cmd_table[i]->init(argc, argv,
							pw_out);
				}
				return true;
			}
		}
	}

	fprintf(stderr, "Unknown command: %s\n", argv[ARG_CMD]);

	return false;
}

void cmd_msg(void *pw, int id, const char *msg, size_t len)
{
	if (cmd_g.cmd == NULL) {
		fprintf(stderr, "%s: cmd uninitialised!\n", __func__);
		return;
	}

	if (cmd_g.cmd->msg != NULL) {
		cmd_g.cmd->msg(pw, id, msg, len);
	}
}

bool cmd_tick(void *pw)
{
	if (cmd_g.cmd == NULL) {
		fprintf(stderr, "%s: cmd uninitialised!\n", __func__);
		return false;
	}

	if (cmd_g.cmd->tick != NULL) {
		return cmd_g.cmd->tick(pw);
	}

	return false;
}

void cmd_fini(void *pw)
{
	if (cmd_g.cmd == NULL) {
		fprintf(stderr, "%s: cmd uninitialised!\n", __func__);
		return;
	}

	if (cmd_g.cmd->fini != NULL) {
		cmd_g.cmd->fini(pw);
	}

	cmd_g.cmd = NULL;
}
