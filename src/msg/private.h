/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (c) 2022 Codethink
 */

#ifndef CDT_MSG_PRIVATE_H
#define CDT_MSG_PRIVATE_H

#include <libwebsockets.h>

struct msg_ctx {
	struct msg_queue queue_send;
	struct msg_queue queue_sent;
};

struct msg_container {
	struct msg_container *prev;
	struct msg_container *next;
	enum msg_type type;
	size_t offset;
	int id;
	char *str;
	size_t len;
	char data[];
};

extern struct msg_ctx msg_g;

/**
 * Variadic function to build message container with internal message string.
 */
bool msg_create(struct msg_container **msg, const char *restrict fmt, ...);

/**
 * Convert from a message string to a message container.
 *
 * \param[in] msg_str  Message to convert.
 * \return the container.
 */
static inline struct msg_container *msg_str_to_container(char *msg_str)
{
	if (msg_str == NULL) {
		return NULL;
	}

	return (void *)(msg_str
			- LWS_SEND_BUFFER_PRE_PADDING
			- offsetof(struct msg_container, data));
}

/**
 * Prototype for message type-specific handler: message to string.
 */
typedef char *(*msg_str_fn)(const struct msg *msg, int id);

/* Handler functions in msg/handler/ .c files. */
char *msg_str_touch_event(const struct msg *msg, int id);

#endif
