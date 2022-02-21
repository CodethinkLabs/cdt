/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (c) 2022 Codethink
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "cmd/cmd.h"
#include "cmd/private.h"

#include "msg/msg.h"

#include "util/cli.h"
#include "util/log.h"
#include "util/file.h"
#include "util/util.h"
#include "util/base64.h"

static struct cmd_screenshot_ctx {
	const char *display;
	const char *format;
	bool finished;
} cmd_screenshot_g = {
	.format = "png",
};

static const struct cli_table_entry cli_entries[] = {
	CMD_CLI_COMMON("screenshot"),
	{
		.p = true,
		.l = "FORMAT",
		.t = CLI_STRING,
		.v.s = &cmd_screenshot_g.format,
		.d = "Image format: png, jpeg (default: png)."
	},
};
static const struct cli_table cli = {
	.entries = cli_entries,
	.count = (sizeof(cli_entries))/(sizeof(*cli_entries)),
	.min_positional = 2,
};

static bool cmd_screenshot_init(int argc, const char **argv,
		struct cmd_options *options, void **pw_out)
{
	int id;

	if (!cmd_cli_parse(argc, argv, &cli, options)) {
		return false;
	}

	cmd_screenshot_g.display = options->display;

	msg_queue_for_send(&(const struct msg)
		{
			.type = MSG_TYPE_CAPTURE_SCREENSHOT,
			.data = {
				.capture_screenshot = {
					.format = cmd_screenshot_g.format,
				},
			},
		}, &id);

	cmd_screenshot_g.finished = false;
	*pw_out = &cmd_screenshot_g;
	return true;
}

static bool get_data(const char *msg, size_t len,
		const char **data_out, size_t *data_len)
{
	const char *marker = "\"data\":\"";
	const char *data;
	const char *end;

	data = strstr(msg, marker);
	if (data == NULL) {
		cdt_log(CDT_LOG_ERROR, "%s: Data not found: %*s",
				__func__, (int)len, msg);
		return false;
	}

	data += strlen(marker);
	end = strchr(data, '"');
	if (end == NULL || end < data) {
		cdt_log(CDT_LOG_ERROR, "%s: Data terminator missing: %*s",
				__func__, (int)len, msg);
		return false;
	}

	*data_len = (size_t)(end - data);
	*data_out = data;
	return true;
}

static void cmd_screenshot_msg(void *pw, int id, const char *msg, size_t len)
{
	struct cmd_screenshot_ctx *ctx = pw;
	const char *data;
	size_t data_len;
	size_t scr_len;
	uint8_t *scr;

	(void)(id);

	if (!get_data(msg, len, &data, &data_len)) {
		ctx->finished = true;
		return;
	}

	if (data_len == 0) {
		cdt_log(CDT_LOG_ERROR, "%s: Zero length screenshot: %*s",
				__func__, (int)len, msg);
		ctx->finished = true;
		return;
	}

	if (!base64_decode(data, data_len, &scr, &scr_len)) {
		cdt_log(CDT_LOG_ERROR, "%s: Base64 decode failed", __func__);
		ctx->finished = true;
		return;
	}

	file_write(scr, scr_len,
			"screenshot-%s.%s",
			str_get_leaf(ctx->display),
			ctx->format);
	free(scr);

	ctx->finished = true;
}

static bool cmd_screenshot_tick(void *pw)
{
	struct cmd_screenshot_ctx *ctx = pw;
	return !ctx->finished;
}

static void cmd_screenshot_help(int argc, const char **argv);

const struct cmd_table cmd_screenshot = {
	.cmd  = "screenshot",
	.init = cmd_screenshot_init,
	.help = cmd_screenshot_help,
	.msg  = cmd_screenshot_msg,
	.tick = cmd_screenshot_tick,
};

static void cmd_screenshot_help(int argc, const char **argv)
{
	cli_help(&cli, (argc > 0) ? argv[0] : "cdt");
}
