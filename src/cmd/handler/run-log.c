/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (c) 2022 Codethink
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <unistd.h>

#include <cyaml/cyaml.h>

#include "cmd/cmd.h"
#include "cmd/private.h"

#include "msg/msg.h"

/* The log messages arrive as an array of arrays as a JSON string.
 * These are the schema to decode that to a `char ***` type. */

static const struct cyaml_schema_value value_entry_schema = {
	CYAML_VALUE_STRING(CYAML_FLAG_POINTER, char *, 0, CYAML_UNLIMITED),
};

static const struct cyaml_schema_value value_inner_schema = {
	CYAML_VALUE_SEQUENCE_FIXED(CYAML_FLAG_POINTER, char *,
			&value_entry_schema, 1),
};

/* Data structure for the log message response data. */
struct message_response {
	int id;
	struct run_log_response {
		struct run_log {
			char *type;
			char *value; /* Log messages in single JSON string */
		} result;
	} result;
};

/* Schema to decode log message response JSON. */

static const struct cyaml_schema_value value_schema = {
	CYAML_VALUE_SEQUENCE(CYAML_FLAG_POINTER, char *,
			&value_inner_schema, 0, CYAML_UNLIMITED),
};

static const struct cyaml_schema_field run_log_fields_schema[] = {
	CYAML_FIELD_STRING_PTR("type", CYAML_FLAG_POINTER,
			struct run_log, type, 0, CYAML_UNLIMITED),
	CYAML_FIELD_STRING_PTR("value", CYAML_FLAG_POINTER,
			struct run_log, value, 0, CYAML_UNLIMITED),
	CYAML_FIELD_END
};

static const struct cyaml_schema_field run_log_response_fields_schema[] = {
	CYAML_FIELD_MAPPING("result", CYAML_FLAG_DEFAULT,
			struct run_log_response, result,
			run_log_fields_schema),
	CYAML_FIELD_END
};

static const struct cyaml_schema_field message_response_fields_schema[] = {
	CYAML_FIELD_STRING_PTR("id", CYAML_FLAG_DEFAULT,
			struct message_response, id, 0, CYAML_UNLIMITED),
	CYAML_FIELD_MAPPING("result", CYAML_FLAG_DEFAULT,
			struct message_response, result,
			run_log_response_fields_schema),
	CYAML_FIELD_END
};

static const struct cyaml_schema_value message_response_schema = {
	CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER, struct message_response,
			message_response_fields_schema),
};

/**
 * JavaScript log capture script.
 * 
 * This makes a copy of the standard JavaScript `console.log()` at
 * `console.stdlog`, and replaces `console.log` with a function that
 * appends the log message to an array, before calling `console.stdlog`.
 *
 * If the log message array already exists, it clears the array.
 */
static const char *log_capture_script =
		"if (console.logs === undefined) {"
		"    console.stdlog = console.log.bind(console);"
		"    console.logs = [];"
		"    console.log = function() {"
		"        console.logs.push(Array.from(arguments));"
		"        console.stdlog.apply(console, arguments);"
		"    }"
		"} else {"
		"    console.logs.length = 0;"
		"}";

/**
 * JavaScript log fetch script.
 */
static const char *log_fetch_script =
		"JSON.stringify(console.logs)";

/**
 * JavaScript log reset script.
 * 
 * Restores standard logging, so that the array doesn't accumulate forever.
 */
static const char *log_reset_script =
		"if (console.logs !== undefined) {"
		"    console.log = console.stdlog;"
		"    console.logs.length = 0;"
		"    console.logs = undefined;"
		"}";

static struct run_log_ctx {
	int id_capture;
	int id_expression;
	int id_fetch;
	int id_reset;

	char ***log;
	unsigned log_count;

	const char *end_marker;
} run_log_g;

static bool cmd_run_log_init(int argc, const char **argv, void **pw_out)
{
	enum {
		ARG_CDT,
		ARG_DISPLAY,
		ARG_RUN_LOG,
		ARG_SCRIPT,
		ARG_END_MARKER,
		ARG__COUNT,
	};

	if (argc < ARG_END_MARKER) {
		fprintf(stderr, "Usage:\n");
		fprintf(stderr, "  %s %s %s <SCRIPT> [END_MARKER]\n",
				argv[ARG_CDT],
				argv[ARG_DISPLAY],
				argv[ARG_RUN_LOG]);
		fprintf(stderr, "\n");
		fprintf(stderr, "  SCRIPT     -- JSON-escaped JavaScript\n");
		fprintf(stderr, "\n");
		fprintf(stderr, "Optional:\n");
		fprintf(stderr, "  END_MARKER -- String indicating end of log\n");
		return false;
	}

	/* Send log capture script. */
	msg_queue_for_send(&(const struct msg)
		{
			.type = MSG_TYPE_EVALUATE,
			.data = {
				.evaluate = {
					.expression = log_capture_script,
				},
			},
		}, &run_log_g.id_capture);

	/* Send expression from command line */
	msg_queue_for_send(&(const struct msg)
		{
			.type = MSG_TYPE_EVALUATE,
			.data = {
				.evaluate = {
					.expression = argv[ARG_SCRIPT],
				},
			},
		}, &run_log_g.id_expression);

	/* Send log fetch script. */
	msg_queue_for_send(&(const struct msg)
		{
			.type = MSG_TYPE_EVALUATE,
			.data = {
				.evaluate = {
					.expression = log_fetch_script,
				},
			},
		}, &run_log_g.id_fetch);

	if (argc >= ARG__COUNT) {
		run_log_g.end_marker = argv[ARG_END_MARKER];
	}

	*pw_out = &run_log_g;
	return true;
}

static const cyaml_config_t config = {
		.flags = CYAML_CFG_IGNORE_UNKNOWN_KEYS,
		.log_level = CYAML_LOG_WARNING,
		.mem_fn = cyaml_mem,
		.log_fn = cyaml_log,
};

static bool cmd_run_log__handle_raw(struct run_log_ctx *ctx, const char *raw)
{
	char ***log;
	bool complete;
	size_t raw_len;
	cyaml_err_t res;
	unsigned log_count;

	if (raw == NULL) {
		return false;
	}

	raw_len = strlen(raw);

	res = cyaml_load_data((const uint8_t *)raw, raw_len,
			&config,
			&value_schema,
			(void **)&log, &log_count);
	if (res != CYAML_OK) {
		fprintf(stderr, "Failed to parse log lines: %s\n",
				cyaml_strerror(res));
		return true;
	}

	if (ctx->log_count > log_count) {
		fprintf(stderr, "Log tamper detected! Got %u, had %u\n",
				log_count, ctx->log_count);
		cyaml_free(&config, &value_schema, log, log_count);
		return true;
	}

	for (unsigned i = 0; i < ctx->log_count; i++) {
		if (log[i][0] != NULL && ctx->log[i][0] != NULL) {
			if (strcmp(log[i][0], ctx->log[i][0]) != 0) {
				fprintf(stderr, "Log tamper detected!\n");
				cyaml_free(&config, &value_schema,
						log, log_count);
				return true;
			}
		}
	}

	for (unsigned i = ctx->log_count; i < log_count; i++) {
		if (log[i][0] != NULL) {
			printf("%s\n", log[i][0]);
		}
	}

	complete = false;
	if (log_count > 0 && log[log_count - 1][0] != NULL) {
		if (ctx->end_marker != NULL) {
			if (strstr(log[log_count - 1][0],
					ctx->end_marker) != NULL) {
				complete = true;
			}
		}
	}

	cyaml_free(&config, &value_schema, ctx->log, ctx->log_count);

	ctx->log = log;
	ctx->log_count = log_count;

	return complete;
}

static void cmd_run_log_msg(void *pw, int id, const char *msg, size_t len)
{
	struct run_log_ctx *ctx = pw;

	if (id != ctx->id_fetch) {
		fprintf(stderr, "Received message with id %i: %*s\n",
				id, (int)len, msg);
		return;

	} else if (id == ctx->id_fetch) {
		char *raw;
		bool complete;
		cyaml_err_t res;
		struct message_response *log_msg;

		res = cyaml_load_data((const uint8_t *)msg, len,
				&config,
				&message_response_schema,
				(void **)&log_msg, NULL);
		if (res != CYAML_OK) {
			fprintf(stderr, "Failed to parse log message: %s\n",
					cyaml_strerror(res));
			return;
		}

		/* Extract raw log message data. */
		raw = log_msg->result.result.value;
		log_msg->result.result.value = NULL;
		cyaml_free(&config, &message_response_schema, log_msg, 0);
		log_msg = NULL;

		complete = cmd_run_log__handle_raw(ctx, raw);
		free(raw);

		/* Send log fetch script. */
		msg_queue_for_send(&(const struct msg)
			{
				.type = MSG_TYPE_EVALUATE,
				.data = {
					.evaluate = {
						.expression = complete ?
							log_reset_script :
							log_fetch_script,
					},
				},
			}, complete ?
				&run_log_g.id_reset :
				&run_log_g.id_fetch);

		if (!complete) {
			sleep(1);
		}
		return;
	}
}

static void cmd_run_log_fini(void *pw)
{
	struct run_log_ctx *ctx = pw;

	cyaml_free(&config, &value_schema, ctx->log, ctx->log_count);
}

const struct cmd_table cmd_run_log = {
	.cmd  = "run-log",
	.init = cmd_run_log_init,
	.msg  = cmd_run_log_msg,
	.fini = cmd_run_log_fini,
};
