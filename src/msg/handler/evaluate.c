/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (c) 2022 Codethink
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "msg/msg.h"
#include "msg/queue.h"
#include "msg/private.h"

#define PRINT_FMT_EVALUATE_START__ID_EXPRESSION \
	"{" \
		"\"id\":%i," \
		"\"method\":\"Runtime.evaluate\"," \
		"\"params\":{" \
			"\"expression\":\"%s\"" \
		"}" \
	"}"

char *msg_str_evaluate(const struct msg *msg, int id)
{
	struct msg_container *m;

	if (!msg_create(&m, PRINT_FMT_EVALUATE_START__ID_EXPRESSION, id,
			msg->data.evaluate.expression)) {
		return NULL;
	}

	m->type = msg->type;
	m->id = id;

	return m->str;
}
