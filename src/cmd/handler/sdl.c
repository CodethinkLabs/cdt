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

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "cmd/cmd.h"
#include "msg/msg.h"
#include "cmd/private.h"

#include "util/cli.h"
#include "util/log.h"
#include "util/file.h"
#include "util/util.h"
#include "util/base64.h"

#define FP_SCALE (1 << 10)

static struct cmd_sdl_ctx {
	SDL_Window   *win;
	SDL_Renderer *ren;

	int window_w;
	int window_h;

	SDL_Texture *frame;
	int frame_w;
	int frame_h;

	SDL_Rect frame_rect;

	int device_w;
	int device_h;
	int device_scale;

	struct {
		bool pressed; /* Whether touch/mouse is pressed. */
		int x_sent;
		int y_sent;
		int x_last;
		int y_last;
		int count;
	} mouse;

} cmd_sdl_g = {
	.window_w = 800,
	.window_h = 600,
};

static const struct cli_table_entry cli_entries[] = {
	CMD_CLI_COMMON("sdl"),
};
static const struct cli_table cli = {
	.entries = cli_entries,
	.count = (sizeof(cli_entries))/(sizeof(*cli_entries)),
	.min_positional = 2,
};

static void cmd_sdl_fini(void *pw)
{
	struct cmd_sdl_ctx *ctx = pw;

	if (ctx->frame != NULL) {
		SDL_DestroyTexture(ctx->frame);
		ctx->frame = NULL;
	}

	if (ctx->ren != NULL) {
		SDL_DestroyRenderer(ctx->ren);
	}
	if (ctx->win != NULL) {
		SDL_DestroyWindow(ctx->win);
	}
	if (SDL_WasInit(SDL_INIT_VIDEO) == SDL_INIT_VIDEO) {
		SDL_Quit();
	}
}

static bool cmd_sdl_init(int argc, const char **argv,
		struct cmd_options *options, void **pw_out)
{
	int id;

	if (!cmd_cli_parse(argc, argv, &cli, options)) {
		return false;
	}

	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		cdt_log(CDT_LOG_ERROR, "SDL_Init Error: %s", SDL_GetError());
		goto error;
	}

	cmd_sdl_g.win = SDL_CreateWindow("Chrome DevTools Tool",
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			cmd_sdl_g.window_w, cmd_sdl_g.window_h,
			SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	if (cmd_sdl_g.win == NULL) {
		cdt_log(CDT_LOG_ERROR, "SDL_CreateWindow Error: %s",
				SDL_GetError());
		goto error;
	}

	cmd_sdl_g.ren = SDL_CreateRenderer(cmd_sdl_g.win, -1,
			SDL_RENDERER_ACCELERATED |
			SDL_RENDERER_PRESENTVSYNC);
	if (cmd_sdl_g.ren == NULL) {
		cdt_log(CDT_LOG_ERROR, "SDL_CreateRenderer Error: %s",
				SDL_GetError());
		goto error;
	}

	if (IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG) {
		cdt_log(CDT_LOG_ERROR, "IMG_Init Error: %s", IMG_GetError());
		goto error;
	}

	if (IMG_Init(IMG_INIT_JPG) != IMG_INIT_JPG) {
		cdt_log(CDT_LOG_ERROR, "IMG_Init Error: %s", IMG_GetError());
		goto error;
	}

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

	msg_queue_for_send(&(const struct msg)
		{
			.type = MSG_TYPE_START_SCREENCAST,
			.data = {
				.start_screencast = {
					.format = "jpeg",
					.max_width = 512,
					.max_height = 512,
				},
			},
		}, &id);

	*pw_out = &cmd_sdl_g;
	return true;

error:
	cmd_sdl_fini(&cmd_sdl_g);
	return false;
}

static void cmd_sdl_msg(void *pw, int id, const char *msg, size_t len)
{
	CDT_UNUSED(pw);
	CDT_UNUSED(id);
	CDT_UNUSED(msg);
	CDT_UNUSED(len);
}

struct scan_ctx {
	const struct msg_scan_spec *spec;
	uint32_t found;

	int session_id;
	size_t data_len;
	const char *data;

	double timestamp;
};

static bool cmd_sdl_msg_scan_cb(
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

	} else if (strcmp(key->key, "deviceWidth") == 0) {
		cmd_sdl_g.device_w = (int)value->integer;

	} else if (strcmp(key->key, "deviceHeight") == 0) {
		cmd_sdl_g.device_h = (int)value->integer;
	}

	return false;
}

static void cmd_sdl__update_frame_rect(struct cmd_sdl_ctx *ctx)
{
	int scale_x = ctx->window_w * FP_SCALE / ctx->frame_w;
	int scale_y = ctx->window_h * FP_SCALE / ctx->frame_h;
	int scale = (scale_x < scale_y) ? scale_x : scale_y;
	int scaled_w = ctx->frame_w * scale / FP_SCALE;
	int scaled_h = ctx->frame_h * scale / FP_SCALE;

	ctx->frame_rect.x = (ctx->window_w - scaled_w) / 2;
	ctx->frame_rect.y = (ctx->window_h - scaled_h) / 2;
	ctx->frame_rect.w = scaled_w;
	ctx->frame_rect.h = scaled_h;

	ctx->device_scale = ctx->device_w * FP_SCALE / scaled_w;
}

static void cmd_sdl__handle_frame(struct cmd_sdl_ctx *ctx,
		uint8_t *data, size_t len)
{
	SDL_Surface *surface;
	SDL_Texture *texture;
	SDL_RWops *ops;
	int height;
	int width;

	ops = SDL_RWFromMem(data, (int)len);
	if (ops == NULL) {
		cdt_log(CDT_LOG_ERROR, "SDL_RWFromMem Error: %s",
				SDL_GetError());
		return;
	}

	surface = IMG_Load_RW(ops, 0);
	SDL_RWclose(ops);
	if (surface == NULL) {
		cdt_log(CDT_LOG_ERROR, "IMG_Load_RW Error: %s", IMG_GetError());
		return;
	}

	height = surface->h;
	width = surface->w;

	texture = SDL_CreateTextureFromSurface(ctx->ren, surface);
	SDL_FreeSurface(surface);
	if (texture == NULL) {
		cdt_log(CDT_LOG_ERROR, "SDL_CreateTextureFromSurface Error: %s",
				SDL_GetError());
		return;
	}

	if (ctx->frame != NULL) {
		SDL_DestroyTexture(ctx->frame);
		ctx->frame = NULL;
	}

	ctx->frame_h = height;
	ctx->frame_w = width;
	ctx->frame = texture;

	cmd_sdl__update_frame_rect(ctx);
}

static void cmd_sdl_evt(void *pw, const char *method, size_t method_len,
		const char *msg, size_t len)
{
	struct cmd_sdl_ctx *ctx = pw;

	if (strncmp(method, "Page.screencastFrame", method_len) == 0) {
		static const struct msg_scan_spec spec[] = {
			{
				.key = "deviceWidth",
				.type = MSG_SCAN_TYPE_INTEGER,
				.depth = 3,
			},
			{
				.key = "deviceHeight",
				.type = MSG_SCAN_TYPE_INTEGER,
				.depth = 3,
			},
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
				cmd_sdl_msg_scan_cb, &scan)) {
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

		cmd_sdl__handle_frame(ctx, scr, scr_len);
		free(scr);
	}
}

static void cmd_sdl__resize(struct cmd_sdl_ctx *ctx, int w, int h)
{
	ctx->window_w = w;
	ctx->window_h = h;
	cmd_sdl__update_frame_rect(ctx);
}

#define CMD_SDL_MOTION_RATE_LIMIT 4

static void cmd_sdl__tap(struct cmd_sdl_ctx *ctx, int x, int y)
{
	int id;
	int scale = ctx->device_scale;
	int frame_x = ctx->frame_rect.x;
	int frame_y = ctx->frame_rect.y;

	if (ctx->mouse.pressed) {
		if (ctx->mouse.x_sent == x &&
		    ctx->mouse.y_sent == y) {
			return;
		}

		/* Rate limit move events. */
		if (ctx->mouse.count++ < CMD_SDL_MOTION_RATE_LIMIT) {
			ctx->mouse.x_last = x;
			ctx->mouse.y_last = y;
			return;
		} else {
			ctx->mouse.count = 0;
		}
	} else {
		cdt_log(CDT_LOG_NOTICE, "Pressed at (%i, %i)",
				(x - frame_x) * scale / FP_SCALE,
				(y - frame_y) * scale / FP_SCALE);
	}

	msg_queue_for_send(&(const struct msg)
		{
			.type = ctx->mouse.pressed ?
					MSG_TYPE_TOUCH_EVENT_MOVE :
					MSG_TYPE_TOUCH_EVENT_START,
			.data = {
				.touch_event = {
					.x = (x - frame_x) * scale / FP_SCALE,
					.y = (y - frame_y) * scale / FP_SCALE,
				},
			},
		}, &id);

	ctx->mouse.x_sent = x;
	ctx->mouse.y_sent = y;
}

static void cmd_sdl__flush_motion(struct cmd_sdl_ctx *ctx)
{
	if (ctx->mouse.pressed && ctx->mouse.count > 0) {
		if (ctx->mouse.x_sent != ctx->mouse.x_last ||
		    ctx->mouse.y_sent != ctx->mouse.y_last) {
			ctx->mouse.count = CMD_SDL_MOTION_RATE_LIMIT;
			cmd_sdl__tap(ctx, ctx->mouse.x_last, ctx->mouse.y_last);
		}
	}
}

static bool cmd_sdl__handle_input(struct cmd_sdl_ctx *ctx)
{
	static SDL_Event event;
	int id;

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_QUIT:
			return false;

		case SDL_KEYDOWN:
			switch (event.key.keysym.sym) {
			case SDLK_ESCAPE:
				return false;
			}
			break;

		case SDL_WINDOWEVENT:
			switch (event.window.event) {
			case SDL_WINDOWEVENT_SIZE_CHANGED:
				cmd_sdl__resize(ctx,
						event.window.data1,
						event.window.data2);
				break;
			}
			break;

		case SDL_MOUSEBUTTONUP:
			cmd_sdl__tap(ctx, event.button.x, event.button.y);
			cmd_sdl__flush_motion(ctx);
			ctx->mouse.pressed = false;
			msg_queue_for_send(&(const struct msg) {
					.type = MSG_TYPE_TOUCH_EVENT_END,
				}, &id);
			break;

		case SDL_MOUSEBUTTONDOWN:
			cmd_sdl__tap(ctx, event.button.x, event.button.y);
			ctx->mouse.pressed = true;
			break;

		case SDL_MOUSEMOTION:
			if (ctx->mouse.pressed) {
				cmd_sdl__tap(ctx,
						event.button.x,
						event.button.y);
			}
			break;

		default:
			break;
		}
	}

	cmd_sdl__flush_motion(ctx);

	return true;
}

static bool cmd_sdl_tick(void *pw)
{
	struct cmd_sdl_ctx *ctx = pw;
	bool running;

	if (ctx->win == NULL || ctx->ren == NULL) {
		return false;
	}

	running = cmd_sdl__handle_input(ctx);
	if (running) {
		SDL_Color bg = {
			.r = 0x0,
			.g = 0x0,
			.b = 0x0,
		};

		SDL_SetRenderDrawColor(ctx->ren, bg.r, bg.g, bg.b, 255);
		SDL_RenderClear(ctx->ren);
		if (ctx->frame != NULL) {
			SDL_RenderCopy(ctx->ren, ctx->frame,
					NULL, &ctx->frame_rect);
		}
		SDL_RenderPresent(ctx->ren);
	}

	return running;
}

static void cmd_sdl_help(int argc, const char **argv);

const struct cmd_table cmd_sdl = {
	.cmd  = "sdl",
	.init = cmd_sdl_init,
	.help = cmd_sdl_help,
	.msg  = cmd_sdl_msg,
	.evt  = cmd_sdl_evt,
	.tick = cmd_sdl_tick,
	.fini = cmd_sdl_fini,
};

static void cmd_sdl_help(int argc, const char **argv)
{
	cli_help(&cli, (argc > 0) ? argv[0] : "cdt");
}
