/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (c) 2022 Codethink
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <cyaml/cyaml.h>

#include "cmd/cmd.h"
#include "cmd/private.h"

#include "msg/msg.h"

#include "util/cli.h"
#include "util/log.h"
#include "util/util.h"
#include "util/decode.h"

/* The log messages arrive as an array of arrays as a JSON string.
 * These are the schema to decode that to a `char ***` type. */

static struct tap_id_ctx {
	const char *id;

	bool got_position;
} tap_id_ctx;

/**
 * Construct a JavaScript script to query an element's position.
 *
 * \param[in] id  Identifies which element to get the position of.
 * \return Script to execute, or NULL on error.
 */
static char *cmd_tap_id__get_pos_script(const char *id)
{
	int written;
	char *str = NULL;
	static const char *id_position_script =
		"JSON.stringify(document"
		"    .getElementById('%s')"
		"    .getBoundingClientRect());";

	written = asprintf(&str, id_position_script, id);
	if (written < 0) {
		return NULL;
	}

	return str;
}

static const struct cli_table_entry cli_entries[] = {
	CMD_CLI_COMMON("tap-id"),
	{
		.p = true,
		.l = "ID",
		.t = CLI_STRING,
		.v.s = &tap_id_ctx.id,
		.d = "An element ID attribute to tap."
	},
};
static const struct cli_table cli = {
	.entries = cli_entries,
	.count = (sizeof(cli_entries))/(sizeof(*cli_entries)),
	.min_positional = 3,
};

static bool cmd_tap_id_init(int argc, const char **argv,
		struct cmd_options *options, void **pw_out)
{
	int id;
	char *script;

	if (!cmd_cli_parse(argc, argv, &cli, options)) {
		return false;
	}

	script = cmd_tap_id__get_pos_script(tap_id_ctx.id);
	if (script == NULL) {
		cdt_log(CDT_LOG_ERROR, "Failed to generate script for id %s",
				tap_id_ctx.id);
		return false;
	}

	/* Send element position acquisition script. */
	msg_queue_for_send(&(const struct msg)
		{
			.type = MSG_TYPE_EVALUATE,
			.data = {
				.evaluate = {
					.expression = script,
				},
			},
		}, &id);

	free(script);
	script = NULL;

	*pw_out = NULL;
	return true;
}

struct element_pos {
	int x;
	int y;
	int w;
	int h;
};

static const struct cyaml_schema_field element_pos_fields_schema[] = {
	CYAML_FIELD_INT("x", CYAML_FLAG_DEFAULT, struct element_pos, x),
	CYAML_FIELD_INT("y", CYAML_FLAG_DEFAULT, struct element_pos, y),
	CYAML_FIELD_INT("width", CYAML_FLAG_DEFAULT, struct element_pos, w),
	CYAML_FIELD_INT("height", CYAML_FLAG_DEFAULT, struct element_pos, h),
	CYAML_FIELD_END
};

static const struct cyaml_schema_value element_pos_schema = {
	CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER, struct element_pos,
			element_pos_fields_schema),
};

static const cyaml_config_t config = {
	.flags = CYAML_CFG_IGNORE_UNKNOWN_KEYS,
	.log_level = CYAML_LOG_WARNING,
	.mem_fn = cyaml_mem,
	.log_fn = cyaml_log,
};

static void cmd_tap_id__do_tap(const char *pos_msg)
{
	struct element_pos *pos;
	cyaml_err_t res;
	int id;

	res = cyaml_load_data((const uint8_t *)pos_msg, strlen(pos_msg),
			&config, &element_pos_schema,
			(void **)&pos, NULL);
	if (res != CYAML_OK) {
		cdt_log(CDT_LOG_ERROR,
				"Failed to parse response: %s",
				cyaml_strerror(res));
		cdt_log(CDT_LOG_ERROR,
				"Error could not locate ID: %s",
				tap_id_ctx.id);
		return;
	}

	tap_id_ctx.got_position = true;

	cdt_log(CDT_LOG_NOTICE, "Tapping '%s' at: (%d, %d)",
			tap_id_ctx.id,
			pos->x + pos->w / 2,
			pos->y + pos->h / 2);

	msg_queue_for_send(&(const struct msg)
		{
			.type = MSG_TYPE_TOUCH_EVENT_START,
			.data = {
				.touch_event = {
					.x = pos->x + pos->w / 2,
					.y = pos->y + pos->h / 2,
				},
			},
		}, &id);

	msg_queue_for_send(&(const struct msg)
		{
			.type = MSG_TYPE_TOUCH_EVENT_END,
		}, &id);

	cyaml_free(&config, &element_pos_schema, pos, 0);
}

static void cmd_tap_id_msg(void *pw, int id, const char *msg, size_t len)
{
	(void)(pw);

	cdt_log(CDT_LOG_INFO, "Received message with id %i: %*s",
			id, (int)len, msg);

	if (!tap_id_ctx.got_position) {
		char *value;

		value = decode_extract_response_value(msg, len);
		if (value == NULL) {
			cdt_log(CDT_LOG_ERROR,
					"Error: Could not locate ID: '%s'",
					tap_id_ctx.id);
			return;
		}

		cmd_tap_id__do_tap(value);
		free(value);
	}
}

static void cmd_tap_id_help(int argc, const char **argv);

const struct cmd_table cmd_tap_id = {
	.cmd  = "tap-id",
	.init = cmd_tap_id_init,
	.help = cmd_tap_id_help,
	.msg  = cmd_tap_id_msg,
};

static void cmd_tap_id_help(int argc, const char **argv)
{
	cli_help(&cli, (argc > 0) ? argv[0] : "cdt");
}
