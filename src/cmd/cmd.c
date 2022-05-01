/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (c) 2022 Codethink
 */

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "cmd/cmd.h"
#include "cmd/private.h"

#include "util/log.h"
#include "util/util.h"

static struct {
	const struct cmd_table *cmd;
} cmd_g;

extern const struct cmd_table cmd_help_table;
extern const struct cmd_table cmd_sdl;
extern const struct cmd_table cmd_run;
extern const struct cmd_table cmd_tap;
extern const struct cmd_table cmd_drag;
extern const struct cmd_table cmd_swipe;
extern const struct cmd_table cmd_tap_id;
extern const struct cmd_table cmd_run_log;
extern const struct cmd_table cmd_screencast;
extern const struct cmd_table cmd_screenshot;

const struct cmd_table *cmd_table[] = {
	&cmd_help_table,
	&cmd_sdl,
	&cmd_run,
	&cmd_tap,
	&cmd_drag,
	&cmd_swipe,
	&cmd_tap_id,
	&cmd_run_log,
	&cmd_screencast,
	&cmd_screenshot,
};

void cmd_print_command_list(void)
{
	fprintf(stderr, "Commands:\n");
	for (size_t i = 0; i < CDT_ARRAY_COUNT(cmd_table); i++) {
		if (cmd_table[i] != NULL) {
			fprintf(stderr, "- %s\n", cmd_table[i]->cmd);
		}
	}
}

bool cmd_init(int argc, const char **argv,
		struct cmd_options *options, void **pw_out)
{
	enum {
		ARG_CDT,
		ARG_CMD,
		ARG__COUNT,
	};

	if (argc < ARG__COUNT) {
		fprintf(stderr, "Usage:\n");
		fprintf(stderr, "  %s <CMD>\n", argv[ARG_CDT]);
		fprintf(stderr, "\n");
		cmd_print_command_list();
		fprintf(stderr, "\n");
		return false;
	}

	for (size_t i = 0; i < CDT_ARRAY_COUNT(cmd_table); i++) {
		if (cmd_table[i] != NULL) {
			if (strcmp(argv[ARG_CMD], cmd_table[i]->cmd) == 0) {
				cmd_g.cmd = cmd_table[i];
				if (cmd_table[i]->init != NULL) {
					return cmd_table[i]->init(argc, argv,
							options, pw_out);
				}
				return true;
			}
		}
	}

	cdt_log(CDT_LOG_ERROR, "Unknown command: %s", argv[ARG_CMD]);

	fprintf(stderr, "\n");
	cmd_print_command_list();
	fprintf(stderr, "\n");

	return false;
}

void cmd_help(int argc, const char **argv, const char *cmd)
{
	if (cmd_g.cmd == NULL) {
		cdt_log(CDT_LOG_ERROR, "%s: cmd uninitialised!", __func__);
		return;
	}

	if (cmd == NULL) {
		enum {
			ARG_CDT,
			ARG_DISPLAY,
			ARG_CMD,
			ARG__COUNT,
		};
		if (argc < ARG__COUNT) {
			cmd = "help";
		} else {
			cmd = argv[ARG_CMD];
		}
	}

	for (size_t i = 0; i < CDT_ARRAY_COUNT(cmd_table); i++) {
		if (cmd_table[i] != NULL) {
			if (strcmp(cmd, cmd_table[i]->cmd) == 0) {
				cmd_g.cmd = cmd_table[i];
				if (cmd_table[i]->help != NULL) {
					cmd_table[i]->help(argc, argv);
				}
				return;
			}
		}
	}

	cdt_log(CDT_LOG_ERROR, "Unknown command: %s", cmd);

	fprintf(stderr, "\n");
	cmd_print_command_list();
	fprintf(stderr, "\n");
}

void cmd_msg(void *pw, int id, const char *msg, size_t len)
{
	if (cmd_g.cmd == NULL) {
		cdt_log(CDT_LOG_ERROR, "%s: cmd uninitialised!", __func__);
		return;
	}

	if (cmd_g.cmd->msg != NULL) {
		cmd_g.cmd->msg(pw, id, msg, len);
	}
}

void cmd_evt(void *pw, const char *method, size_t method_len,
		const char *msg, size_t len)
{
	if (cmd_g.cmd == NULL) {
		cdt_log(CDT_LOG_ERROR, "%s: cmd uninitialised!", __func__);
		return;
	}

	if (cmd_g.cmd->evt != NULL) {
		cmd_g.cmd->evt(pw, method, method_len, msg, len);
	}
}

bool cmd_tick(void *pw)
{
	if (cmd_g.cmd == NULL) {
		cdt_log(CDT_LOG_ERROR, "%s: cmd uninitialised!", __func__);
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
		cdt_log(CDT_LOG_ERROR, "%s: cmd uninitialised!", __func__);
		return;
	}

	if (cmd_g.cmd->fini != NULL) {
		cmd_g.cmd->fini(pw);
	}

	cmd_g.cmd = NULL;
}
