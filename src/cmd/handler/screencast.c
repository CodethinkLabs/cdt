/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (c) 2022 Codethink
 */

#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <unistd.h>

#include "cmd/cmd.h"
#include "msg/msg.h"
#include "cmd/private.h"

#include "util/log.h"
#include "util/file.h"
#include "util/util.h"
#include "util/base64.h"

static struct cmd_screencast_ctx {
	const char *display;
	const char *format;
} cmd_screencast_g;

static bool cmd_screencast_init(int argc, const char **argv, void **pw_out)
{
	int id;
	int w = 0;
	int h = 0;
	const char *format = "jpeg";
	enum {
		ARG_CDT,
		ARG_DISPLAY,
		ARG_SCREENCAST,
		ARG_MAX_SIZE,
		ARG__COUNT,
	};

	if (argc < ARG_MAX_SIZE || argc > ARG__COUNT) {
		cmd_help(argc, argv, NULL);
		return false;
	}

	if (argc > ARG_MAX_SIZE) {
		w = atoi(argv[ARG_MAX_SIZE]);
		h = atoi(argv[ARG_MAX_SIZE]);
	}

	msg_queue_for_send(&(const struct msg)
		{
			.type = MSG_TYPE_START_SCREENCAST,
			.data = {
				.start_screencast = {
					.max_width = w,
					.max_height = h,
					.format = format,
				},
			},
		}, &id);

	cmd_screencast_g.format = format;
	cmd_screencast_g.display = argv[ARG_DISPLAY];

	*pw_out = &cmd_screencast_g;
	return true;
}

static void cmd_screencast_msg(void *pw, int id, const char *msg, size_t len)
{
	(void)(pw);

	cdt_log(CDT_LOG_NOTICE, "Received response with id %i: %.*s",
			id, (int)len, msg);
}

struct scan_ctx {
	const struct msg_scan_spec *spec;
	uint32_t found;

	int session_id;
	size_t data_len;
	const char *data;

	double timestamp;
};

static bool cmd_screencast_msg_scan_cb(
		void *pw,
		const struct msg_scan_spec *key,
		const union  msg_scan_data *value)
{
	struct scan_ctx *scan = pw;

	assert(key != NULL);
	assert(value != NULL);

	scan->found |= 1 << (key - scan->spec);

	if (strcmp(key->key, "sessionId") == 0) {
		scan->session_id = (int)value->integer;

	} else if (strcmp(key->key, "data") == 0) {
		scan->data_len = value->string.len;
		scan->data = value->string.str;

	} else if (strcmp(key->key, "timestamp") == 0) {
		scan->timestamp = value->floating_point;
	}

	return false;
}

static void cmd_screencast_evt(void *pw, const char *method, size_t method_len,
		const char *msg, size_t len)
{
	struct cmd_screencast_ctx *ctx = pw;

	cdt_log(CDT_LOG_NOTICE, "Received event with method: %.*s",
			(int)method_len, method);

	if (strncmp(method, "Page.screencastFrame", method_len) == 0) {
		static const struct msg_scan_spec spec[] = {
			{
				.key = "timestamp",
				.type = MSG_SCAN_TYPE_FLOATING_POINT,
				.depth = 3,
			},
			{
				.key = "sessionId",
				.type = MSG_SCAN_TYPE_INTEGER,
				.depth = 2,
			},
			{
				.key = "data",
				.type = MSG_SCAN_TYPE_STRING,
				.depth = 2,
			},
		};
		enum {
			FOUND_MASK = UINT32_MAX >> (32 - CDT_ARRAY_COUNT(spec)),
		};
		struct scan_ctx scan = {
			.spec = spec,
			.found = 0,
		};
		size_t scr_len;
		uint8_t *scr;
		int id;

		if (!msg_str_scan(msg, len,
				spec, CDT_ARRAY_COUNT(spec),
				cmd_screencast_msg_scan_cb, &scan)) {
			cdt_log(CDT_LOG_ERROR,
					"%s: Failed to scan message: %*s",
					__func__, (int)method_len, method);
			return;
		}

		if (scan.found != FOUND_MASK) {
			cdt_log(CDT_LOG_ERROR,
					"%s: Message missing components: %*s",
					__func__, (int)method_len, method);
			return;
		}

		msg_queue_for_send(&(const struct msg)
			{
				.type = MSG_TYPE_SCREENCAST_FRAME_ACK,
				.data = {
					.screencast_frame_ack = {
						.session_id = scan.session_id,
					},
				},
			}, &id);

		if (!base64_decode(
				scan.data,
				scan.data_len,
				&scr, &scr_len)) {
			cdt_log(CDT_LOG_ERROR, "%s: Base64 decode failed",
					__func__);
			return;
		}

		file_write(scr, scr_len,
				"screenshot-%s-%.6f.%s",
				str_get_leaf(ctx->display),
				scan.timestamp,
				ctx->format);
		free(scr);
	}
}

static bool cmd_screencast_tick(void *pw)
{
	(void)(pw);
	usleep(1000 * 10);
	return true;
}

static void cmd_screencast_help(int argc, const char **argv);

const struct cmd_table cmd_screencast = {
	.cmd  = "screencast",
	.init = cmd_screencast_init,
	.help = cmd_screencast_help,
	.msg  = cmd_screencast_msg,
	.evt  = cmd_screencast_evt,
	.tick = cmd_screencast_tick,
};

static void cmd_screencast_help(int argc, const char **argv)
{
	enum {
		ARG_CDT,
		ARG_DISPLAY,
		ARG__COUNT,
	};

	CDT_UNUSED(argc);

	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "  %s %s %s [MAX_SIZE]\n",
			argv[ARG_CDT],
			argv[ARG_DISPLAY],
			cmd_screencast.cmd);
	fprintf(stderr, "\n");
	fprintf(stderr, "Optional:\n");
	fprintf(stderr, "  MAX_SIZE -- Maximum x/y dimension in px\n");
}
