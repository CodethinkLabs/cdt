/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (c) 2022 Codethink
 */

#ifndef CDT_CMD_PRIVATE_H
#define CDT_CMD_PRIVATE_H

#include "util/cli.h"
#include "util/log.h"

/**
 * Command implementation vtable.
 */
struct cmd_table {
	const char *cmd;

	void (*help)(int argc, const char **argv);
	bool (*init)(int argc, const char **argv,
			struct cmd_options *options, void **pw_out);
	void (*msg) (void *pw, int id, const char *msg, size_t len);
	void (*evt) (void *pw, const char *method, size_t method_len,
			const char *msg, size_t len);
	bool (*tick)(void *pw);
	void (*fini)(void *pw);
};

static struct cmd_options cmd_options;

static const struct cli_str_val cmd_cli_common_log_level[] = {
	{ .str = "debug"  , .val = CDT_LOG_DEBUG  , },
	{ .str = "info"   , .val = CDT_LOG_INFO   , },
	{ .str = "notice" , .val = CDT_LOG_NOTICE , },
	{ .str = "warning", .val = CDT_LOG_WARNING, },
	{ .str = "error"  , .val = CDT_LOG_ERROR  , },
	{ .str = NULL, },
};

static const struct cli_str_val cmd_cli_common_log_target[] = {
	{ .str = "stdout", .val = CDT_LOG_STDOUT, },
	{ .str = "stderr", .val = CDT_LOG_STDERR, },
	{ .str = "syslog", .val = CDT_LOG_SYSLOG, },
	{ .str = NULL, },
};

#define CMD_CLI_COMMON(_cmd) \
	{ \
		.p = true, \
		.l = _cmd, \
		.t = CLI_CMD, \
	}, \
	{ \
		.p = true, \
		.l = "DISPLAY", \
		.t = CLI_STRING, \
		.v.s = &cmd_options.display, \
		.d = "Identifier for browser context to connect to." \
	}, \
	{ \
		.s = 'l', \
		.l = "log-level", \
		.t = CLI_ENUM, \
		.v.e.e = &cmd_options.log_level, \
		.v.e.desc = cmd_cli_common_log_level, \
		.d = "Log level (error, warning, notice, info, debug)." \
	}, \
	{ \
		.s = 't', \
		.l = "log-target", \
		.t = CLI_ENUM, \
		.v.e.e = &cmd_options.log_target, \
		.v.e.desc = cmd_cli_common_log_target, \
		.d = "Logging target (stdout, stderr, syslog)." \
	}

static inline bool cmd_cli_parse(int argc, const char **argv,
		const struct cli_table *cli, struct cmd_options *options)
{
	cmd_options = *options;
	if (!cli_parse(cli, argc, argv)) {
		cdt_log(CDT_LOG_ERROR, "Failed to parse command line");
		cmd_help(argc, argv, cli->entries[0].l);
		return false;
	}

	*options = cmd_options;
	return true;
}

#endif
