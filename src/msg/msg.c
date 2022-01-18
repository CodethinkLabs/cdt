/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (c) 2022 Codethink
 */

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "msg/msg.h"
#include "msg/queue.h"
#include "msg/private.h"

#include "util/util.h"

struct msg_ctx msg_g;

bool msg_create(struct msg_container **msg, const char *restrict fmt, ...)
{
	int ret;
	bool res;
	size_t size;
	va_list args;
	va_list args2;
	struct msg_container *cont;

	res = false;

	va_start(args, fmt);
	va_copy(args2, args);

	ret = vsnprintf(NULL, 0, fmt, args);
	va_end(args);

	if (ret <= 0) {
		fprintf(stderr, "%s: Message length failed!\n", __func__);
		goto error;
	}

	size = offsetof(struct msg_container, data)
			+ LWS_SEND_BUFFER_PRE_PADDING
			+ (unsigned)ret + 1
			+ LWS_SEND_BUFFER_POST_PADDING;
	cont = calloc(size, 1);
	if (cont == NULL) {
		fprintf(stderr, "%s: Allocation failed!\n", __func__);
		goto error;
	}

	cont->len = (unsigned)ret;
	cont->str = ((char *)(cont))
			+ offsetof(struct msg_container, data)
			+ LWS_SEND_BUFFER_PRE_PADDING;

	if (vsnprintf(cont->str, (unsigned)ret + 1, fmt, args2) != ret) {
		free(cont);
		fprintf(stderr, "%s: Message length unexpected!\n", __func__);
		goto error;
	}

	*msg = cont;
	res = true;
error:
	va_end(args2);
	return res;
}

void msg_destroy(char *msg)
{
	if (msg != NULL) {
		free(msg_str_to_container(msg));
	}
}

size_t msg_get_len(char *msg_str)
{
	struct msg_container *msg = msg_str_to_container(msg_str);
	return msg->len;
}

bool msg_to_msg_str(const struct msg *msg, char **msg_str, int *id_out)
{
	static uint16_t id;

	msg_str_fn msg_stringify[] = {
		[MSG_TYPE_EVALUATE]             = msg_str_evaluate,
		[MSG_TYPE_TOUCH_EVENT_END]      = msg_str_touch_event,
		[MSG_TYPE_TOUCH_EVENT_MOVE]     = msg_str_touch_event,
		[MSG_TYPE_TOUCH_EVENT_START]    = msg_str_touch_event,
		[MSG_TYPE_SCROLL_GESTURE]       = msg_str_scroll_gesture,
		[MSG_TYPE_START_SCREENCAST]     = msg_str_start_screencast,
		[MSG_TYPE_CAPTURE_SCREENSHOT]   = msg_str_capture_screenshot,
		[MSG_TYPE_SCREENCAST_FRAME_ACK] = msg_str_screencast_frame_ack,
	};

	if (msg->type >= CDT_ARRAY_COUNT(msg_stringify)) {
		fprintf(stderr, "%s: Message type not handled: %i\n",
				__func__, msg->type);
		return false;
	}

	*msg_str = msg_stringify[msg->type](msg, id);
	if (*msg_str == NULL) {
		fprintf(stderr, "%s: Failed to assemble message type: %i\n",
				__func__, msg->type);
		return false;
	}

	*id_out = id++;
	return true;
}

bool msg_queue_for_send(const struct msg *msg, int *id_out)
{
	char *msg_str;

	if (!msg_to_msg_str(msg, &msg_str, id_out)) {
		return false;
	}

	msg_queue_push(msg_queue_get_send(), msg_str);
	return true;
}

struct msg_str_ctx {
	bool quote;
	bool begin;
	int depth;
	int id;
};

static void msg_str_chunk_scan_reset(struct msg_str_ctx *c)
{
	memset(c, 0, sizeof(*c));
}

enum msg_scan msg_str_chunk_scan(const char *str, size_t len)
{
	static struct msg_str_ctx c;
	const char *end = str + len;

	if (c.depth == 0) {
		if (str == NULL || len == 0 || str[0] != '{') {
			return MSG_SCAN_ERROR;
		}
	}

	while (str < end) {
		switch (*str) {
		case '\\': str++; c.begin = false; break;
		case ',': if (!c.quote) { c.begin = true;            } break;
		case '[': if (!c.quote) { c.begin = true; c.depth++; } break;
		case '{': if (!c.quote) { c.begin = true; c.depth++; } break;
		case ']': if (!c.quote) {                 c.depth--; } break;
		case '}': if (!c.quote) {                 c.depth--; } break;
		case '"':
			c.quote = !c.quote;
			/* Fall through. */
		default:
			c.begin = false;
			break;
		}

		str++;
	}

	if (c.depth != 0) {
		/* More message required. */
		return MSG_SCAN_CONTINUE;
	}

	msg_str_chunk_scan_reset(&c);
	return MSG_SCAN_COMPLETE;
}

static bool msg_str_get_value_len(
		const char *str, const char *end,
		enum msg_scan_type type,
		size_t *len)
{
	const char *pos = str;

	switch (type) {
	case MSG_SCAN_TYPE_INTEGER:
	case MSG_SCAN_TYPE_FLOATING_POINT:
		if (pos + 1 >= end || pos[0] == '"') {
			return false;
		}
		while (pos < end) {
			if (*pos == '\\') {
				pos += 2;
				continue;
			} else if (*pos == ',' || *pos == '}' || *pos == ']') {
				*len = (size_t)(pos - str);
				return true;
			} else if (*pos == '"') {
				return false;
			}
			pos++;
		}
		break;
	case MSG_SCAN_TYPE_STRING:
		if (pos + 1 >= end || pos[0] != '"') {
			return false;
		}
		pos++;
		str++;

		while (pos < end) {
			if (*pos == '\\') {
				pos += 2;
				continue;
			} else if (*pos == '"') {
				*len = (size_t)(pos - str);
				return true;
			} else if (*pos == ',' || *pos == '}' || *pos == ']') {
				return false;
			}
			pos++;
		}
		break;
	}

	return false;
}

static const struct msg_scan_spec *msg_str_scan_get_match(
		const char *str, const char *end,
		int depth,
		const struct msg_scan_spec *spec,
		unsigned spec_count,
		union msg_scan_data *value)
{
	size_t value_len;

	assert(str < end && *str == '"');

	str++;
	if (str == end) {
		return NULL;
	}

	for (unsigned i = 0; i < spec_count; i++) {
		const struct msg_scan_spec *s = &spec[i];
		const char *pos = str;
		size_t key_len;

		if (s->depth != 0 && s->depth != depth) {
			continue;
		}

		key_len = strlen(s->key);
		if (pos + key_len >= end) {
			continue;
		}

		if (strncmp(pos, s->key, key_len) != 0) {
			continue;
		}

		pos += key_len;
		if (pos + 2 >= end ||
		    pos[0] != '"' ||
		    pos[1] != ':') {
			continue;
		}
		pos += 2;

		if (!msg_str_get_value_len(pos, end, s->type, &value_len)) {
			continue;
		}

		switch (s->type) {
		case MSG_SCAN_TYPE_FLOATING_POINT:
			{
				double temp;
				char *fin = NULL;

				errno = 0;
				temp = strtod(pos, &fin);

				if (fin == pos || errno == ERANGE) {
					return NULL;
				}

				value->floating_point = temp;
			}
			return s;

		case MSG_SCAN_TYPE_INTEGER:
			{
				long long temp;
				char *fin = NULL;

				errno = 0;
				temp = strtoll(pos, &fin, 0);

				if (fin == pos || errno == ERANGE ||
				    temp < INT64_MIN || temp > INT64_MAX) {
					return NULL;
				}

				value->integer = temp;
			}
			return s;

		case MSG_SCAN_TYPE_STRING:
			value->string.str = pos + 1;
			value->string.len = value_len;
			return s;
		}
	}

	return NULL;
}

bool msg_str_scan(const char *str, size_t len,
		const struct msg_scan_spec *spec, unsigned spec_count,
		msg_scan_cb cb, void *pw)
{
	struct msg_str_ctx c = { 0 };
	const char *end = str + len;

	if (c.depth == 0) {
		if (str == NULL || len == 0 || str[0] != '{') {
			return false;
		}
	}

	while (str < end) {
		switch (*str) {
		case '\\': str++; c.begin = false; break;
		case ',': if (!c.quote) { c.begin = true;            } break;
		case '[': if (!c.quote) { c.begin = true; c.depth++; } break;
		case '{': if (!c.quote) { c.begin = true; c.depth++; } break;
		case ']': if (!c.quote) {                 c.depth--; } break;
		case '}': if (!c.quote) {                 c.depth--; } break;
		case '"':
			if (!c.quote && c.begin) {
				const struct msg_scan_spec *key;
				union msg_scan_data value;

				key = msg_str_scan_get_match(str, end, c.depth,
						spec, spec_count, &value);
				if (key != NULL) {
					bool finish = cb(pw, key, &value);
					if (finish) {
						goto out;
					}
				}
			}
			c.quote = !c.quote;
			/* Fall through. */
		default:
			c.begin = false;
			break;
		}

		str++;
	}

out:
	return true;
}
