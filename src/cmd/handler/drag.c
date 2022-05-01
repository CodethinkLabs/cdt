/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (c) 2022 Michael Drake <tlsa@netsurf-browser.org>
 */

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <unistd.h>

#include "cmd/cmd.h"
#include "cmd/private.h"

#include "msg/msg.h"

#include "util/cli.h"
#include "util/log.h"
#include "util/time.h"
#include "util/util.h"

static struct drag_ctx {
	int64_t x0;
	int64_t y0;
	int64_t x1;
	int64_t y1;

	int64_t steps;
	int64_t duration;

	int64_t step;
	struct timespec time_start;
} drag_ctx = {
	.steps = 10,
	.duration = 500,
};

static const struct cli_table_entry cli_entries[] = {
	CMD_CLI_COMMON("drag"),
	{
		.p = true,
		.l = "START_X",
		.t = CLI_INT,
		.v.i = &drag_ctx.x0,
		.d = "X coordinate to drag from (px from top edge)."
	},
	{
		.p = true,
		.l = "START_Y",
		.t = CLI_INT,
		.v.i = &drag_ctx.y0,
		.d = "Y coordinate to drag from (px from left edge)."
	},
	{
		.p = true,
		.l = "END_X",
		.t = CLI_INT,
		.v.i = &drag_ctx.x1,
		.d = "X coordinate to drag to (px from top edge)."
	},
	{
		.p = true,
		.l = "END_Y",
		.t = CLI_INT,
		.v.i = &drag_ctx.y1,
		.d = "Y coordinate to drag to (px from left edge)."
	},
	{
		.s = 's',
		.l = "steps",
		.t = CLI_INT,
		.v.i = &drag_ctx.steps,
		.d = "Number of drag steps. The default is 10."
	},
	{
		.s = 'd',
		.l = "duration",
		.t = CLI_INT,
		.v.i = &drag_ctx.duration,
		.d = "Duration of drag (ms). The default is 500."
	},
};
static const struct cli_table cli = {
	.entries = cli_entries,
	.count = (sizeof(cli_entries))/(sizeof(*cli_entries)),
	.min_positional = 6,
};

static int lerp(int64_t a, int64_t b)
{
	return (int)(a + (b - a) * drag_ctx.step / drag_ctx.steps);
}

static bool cmd_drag_init(int argc, const char **argv,
		struct cmd_options *options, void **pw_out)
{
	int ret;
	int id;

	if (!cmd_cli_parse(argc, argv, &cli, options)) {
		return false;
	}

	if (drag_ctx.steps == 0) {
		drag_ctx.steps++;
	}

	ret = clock_gettime(CLOCK_MONOTONIC, &drag_ctx.time_start);
	if (ret == -1) {
		return false;
	}

	msg_queue_for_send(&(const struct msg)
		{
			.type = MSG_TYPE_TOUCH_EVENT_START,
			.data = {
				.touch_event = {
					.x = lerp(drag_ctx.x0, drag_ctx.x1),
					.y = lerp(drag_ctx.y0, drag_ctx.y1),
				},
			},
		}, &id);

	*pw_out = &drag_ctx;
	return true;
}

static void cmd_drag_msg(void *pw, int id, const char *msg, size_t len)
{
	(void)(pw);

	cdt_log(CDT_LOG_NOTICE, "Received message with id %i: %*s",
			id, (int)len, msg);
}

static bool cmd_drag_tick(void *pw)
{
	struct drag_ctx *ctx = pw;
	struct timespec time_now;
	int64_t time_passed;
	int64_t time_step;
	int ret;
	int id;

	time_step = ctx->duration * (ctx->step + 1) / ctx->steps;
	if (time_step > ctx->duration) {
		time_step = ctx->duration;
	}

	ret = clock_gettime(CLOCK_MONOTONIC, &time_now);
	if (ret == -1) {
		time_passed = time_step;
	} else {
		time_passed = time_diff_ms(&ctx->time_start, &time_now);
	}

	if (time_passed >= time_step) {
		ctx->step++;
		if (ctx->step <= ctx->steps) {
			msg_queue_for_send(&(const struct msg)
				{
					.type = MSG_TYPE_TOUCH_EVENT_MOVE,
					.data = {
						.touch_event = {
							.x = lerp(ctx->x0,
							          ctx->x1),
							.y = lerp(ctx->y0,
							          ctx->y1),
						},
					},
				}, &id);
		}

		if (ctx->step == ctx->steps) {
			msg_queue_for_send(&(const struct msg)
				{
					.type = MSG_TYPE_TOUCH_EVENT_END,
				}, &id);
		}
	} else {
		int64_t sleep_ms = 10 < time_step - time_passed ?
		                   10 : time_step - time_passed;

		usleep((useconds_t)(1000 * sleep_ms));
	}

	return ctx->step <= ctx->steps;
}

static void cmd_drag_help(int argc, const char **argv);

const struct cmd_table cmd_drag = {
	.cmd  = "drag",
	.init = cmd_drag_init,
	.help = cmd_drag_help,
	.msg  = cmd_drag_msg,
	.tick = cmd_drag_tick,
};

static void cmd_drag_help(int argc, const char **argv)
{
	cli_help(&cli, (argc > 0) ? argv[0] : "cdt");
}
