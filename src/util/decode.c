/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (c) 2022 Codethink
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <cyaml/cyaml.h>

#include "util/log.h"
#include "util/decode.h"

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

static cyaml_config_t config = {
	.flags = CYAML_CFG_IGNORE_UNKNOWN_KEYS,
	.log_level = CYAML_LOG_WARNING,
	.mem_fn = cyaml_mem,
	.log_fn = cyaml_log,
};

char *decode_extract_response_value(const char *msg, size_t len)
{
	struct message_response *response;
	cyaml_err_t res;
	char *value;

	config.log_ctx = (void *) cdt_log_get_level();

	res = cyaml_load_data((const uint8_t *)msg, len,
			&config,
			&message_response_schema,
			(void **)&response, NULL);
	if (res != CYAML_OK) {
		cdt_log(CDT_LOG_ERROR,
				"Failed to parse response: %s",
				cyaml_strerror(res));
		return NULL;
	}

	if (strcmp(response->result.result.type, "string") != 0) {
		cdt_log(CDT_LOG_WARNING, "Expecting value of type 'string'");
	}

	/* Extract the value. */
	value = response->result.result.value;
	response->result.result.value = NULL;

	cyaml_free(&config, &message_response_schema, response, 0);
	response = NULL;

	return value;
}

/* Data structure for the log message response data. */
struct message_response_int {
	int id;
	struct run_log_response_int {
		struct run_log_int {
			char *type;
			int value; /* Log messages in single JSON string */
		} result;
	} result;
};

/* Schema to decode log message response JSON. */

static const struct cyaml_schema_field run_log_fields_int_schema[] = {
	CYAML_FIELD_STRING_PTR("type", CYAML_FLAG_POINTER,
			struct run_log_int, type, 0, CYAML_UNLIMITED),
	CYAML_FIELD_INT("value", CYAML_FLAG_POINTER, struct run_log_int, value),
	CYAML_FIELD_END
};

static const struct cyaml_schema_field run_log_response_int_fields_schema[] = {
	CYAML_FIELD_MAPPING("result", CYAML_FLAG_DEFAULT,
			struct run_log_response_int, result,
			run_log_fields_int_schema),
	CYAML_FIELD_END
};

static const struct cyaml_schema_field message_response_int_fields_schema[] = {
	CYAML_FIELD_STRING_PTR("id", CYAML_FLAG_DEFAULT,
			struct message_response_int, id, 0, CYAML_UNLIMITED),
	CYAML_FIELD_MAPPING("result", CYAML_FLAG_DEFAULT,
			struct message_response_int, result,
			run_log_response_int_fields_schema),
	CYAML_FIELD_END
};

static const struct cyaml_schema_value message_response_int_schema = {
	CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER, struct message_response_int,
			message_response_int_fields_schema),
};

bool decode_extract_response_value_int(const char *msg, size_t len, int *ret)
{
	struct message_response_int *response;
	cyaml_err_t res;

	config.log_ctx = (void *) cdt_log_get_level();

	res = cyaml_load_data((const uint8_t *)msg, len,
			&config,
			&message_response_int_schema,
			(void **)&response, NULL);
	if (res != CYAML_OK) {
		cdt_log(CDT_LOG_ERROR,
				"Failed to parse response: %s",
				cyaml_strerror(res));
		return false;
	}

	if (strcmp(response->result.result.type, "number") != 0) {
		cdt_log(CDT_LOG_WARNING, "Expecting value of type 'number'");
	}

	/* Extract the value. */
	*ret = response->result.result.value;

	cyaml_free(&config, &message_response_int_schema, response, 0);
	response = NULL;

	return true;
}
