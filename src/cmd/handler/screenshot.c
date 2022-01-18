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
#include "util/file.h"
#include "util/util.h"
#include "util/base64.h"

static struct cmd_screenshot_ctx {
	const char *display;
	const char *format;
	bool finished;
} cmd_screenshot_g;

static bool cmd_screenshot_init(int argc, const char **argv, void **pw_out)
{
	const char *fmt = "png";
	int id;
	enum {
		ARG_CDT,
		ARG_DISPLAY,
		ARG_SCREENSHOT,
		ARG_FMT,
		ARG__COUNT,
	};

	if (argc < ARG_FMT || argc > ARG__COUNT) {
		cmd_help(argc, argv, NULL);
		return false;
	}

	if (argc > ARG_FMT) {
		fmt = argv[ARG_FMT];
	}

	msg_queue_for_send(&(const struct msg)
		{
			.type = MSG_TYPE_CAPTURE_SCREENSHOT,
			.data = {
				.capture_screenshot = {
					.format = fmt,
				},
			},
		}, &id);

	cmd_screenshot_g.format = fmt;
	cmd_screenshot_g.display = argv[ARG_DISPLAY];
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
		fprintf(stderr, "%s: Data not found: %*s\n",
				__func__, (int)len, msg);
		return false;
	}

	data += strlen(marker);
	end = strchr(data, '"');
	if (end == NULL || end < data) {
		fprintf(stderr, "%s: Data terminator missing: %*s\n",
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
		fprintf(stderr, "%s: Zero length screenshot: %*s\n",
				__func__, (int)len, msg);
		ctx->finished = true;
		return;
	}

	if (!base64_decode(data, data_len, &scr, &scr_len)) {
		fprintf(stderr, "%s: Base64 decode failed\n", __func__);
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
	enum {
		ARG_CDT,
		ARG_DISPLAY,
		ARG__COUNT,
	};

	CDT_UNUSED(argc);

	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "  %s %s %s [FMT]\n",
			argv[ARG_CDT],
			argv[ARG_DISPLAY],
			cmd_screenshot.cmd);
	fprintf(stderr, "\n");
	fprintf(stderr, "Optional:\n");
	fprintf(stderr, "  FMT -- Image format: png, jpeg\n");
}
