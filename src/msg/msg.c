/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (c) 2022 Codethink
 */

#include <stddef.h>
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
		[MSG_TYPE_TOUCH_EVENT_END]    = msg_str_touch_event,
		[MSG_TYPE_TOUCH_EVENT_START]  = msg_str_touch_event,
		[MSG_TYPE_CAPTURE_SCREENSHOT] = msg_str_capture_screenshot,
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
	bool have_id;
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

bool msg_str_id(const char *str, size_t len, int *id)
{
	static struct msg_str_ctx c;
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
			if (!c.quote && c.begin && c.depth == 1) {
				int ret = sscanf(str, "\"id\":%i", &c.id);
				if (ret == 1 && str != end - 1) {
					c.have_id = true;
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

	if (c.depth != 0) {
		return false;
	}
	if (!c.have_id) {
		fprintf(stderr, "Failed to find message ID\n");
		msg_str_chunk_scan_reset(&c);
		return false;
	}

	*id = c.id;
	msg_str_chunk_scan_reset(&c);
	return true;
}
