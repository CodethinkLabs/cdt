/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (c) 2022 Codethink
 */

#include <string.h>
#include <stdbool.h>

#include <cyaml/cyaml.h>
#include <libwebsockets.h>

#include "display.h"

#include "msg/msg.h"

#include "util/log.h"
#include "util/buffer.h"

struct display {
	char *description;
	char *frontend_url;
	char *id;
	char *title;
	char *type;
	char *url;
	char *parent_id;
	char *debugger_url;
};

static const struct cyaml_schema_field display_fields_schema[] = {
	CYAML_FIELD_STRING_PTR("description",
			CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
			struct display, description, 0, CYAML_UNLIMITED),
	CYAML_FIELD_STRING_PTR("devtoolsFrontendUrl",
			CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
			struct display, frontend_url, 0, CYAML_UNLIMITED),
	CYAML_FIELD_STRING_PTR("id",
			CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
			struct display, id, 0, CYAML_UNLIMITED),
	CYAML_FIELD_STRING_PTR("title", CYAML_FLAG_POINTER,
			struct display, title, 0, CYAML_UNLIMITED),
	CYAML_FIELD_STRING_PTR("type", CYAML_FLAG_POINTER,
			struct display, type, 0, CYAML_UNLIMITED),
	CYAML_FIELD_STRING_PTR("url",
			CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
			struct display, url, 0, CYAML_UNLIMITED),
	CYAML_FIELD_STRING_PTR("webSocketDebuggerUrl", CYAML_FLAG_POINTER,
			struct display, debugger_url, 0, CYAML_UNLIMITED),
	CYAML_FIELD_STRING_PTR("parentId",
			CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
			struct display, parent_id, 0, CYAML_UNLIMITED),
	CYAML_FIELD_END
};

static const struct cyaml_schema_value display_entry_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_DEFAULT, struct display,
				display_fields_schema),
};

static const struct cyaml_schema_value display_schema = {
		CYAML_VALUE_SEQUENCE(CYAML_FLAG_POINTER, struct display,
				&display_entry_schema, 0, CYAML_UNLIMITED),
};

static struct {
	bool interrupted;
	struct lws *client_wsi;

	struct cdt_buffer multipart_msg;

	struct display *display;
	unsigned display_count;
} display_g;

static int http_cb(struct lws *wsi, enum lws_callback_reasons reason,
		void *user, void *in, size_t len)
{
	unsigned status;

	switch (reason) {
	case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
		cdt_log(CDT_LOG_ERROR, "Disconnected");
		display_g.client_wsi = NULL;
		break;

	case LWS_CALLBACK_ESTABLISHED_CLIENT_HTTP:
		status = lws_http_client_http_response(wsi);
		cdt_log(CDT_LOG_INFO, "Connected with server response: %d", status);
		break;

	case LWS_CALLBACK_RECEIVE_CLIENT_HTTP_READ:
		if (!cdt_buffer_append(&display_g.multipart_msg, in, len)) {
			return false;
		}
		return 0; /* don't passthru */

	case LWS_CALLBACK_RECEIVE_CLIENT_HTTP:
		{
			char buffer[256 + LWS_PRE];
			char *px = buffer + LWS_PRE;
			int lenx = sizeof(buffer) - LWS_PRE;

			if (lws_http_client_read(wsi, &px, &lenx) < 0)
				return -1;
		}
		return 0; /* don't passthru */

	case LWS_CALLBACK_COMPLETED_CLIENT_HTTP:
	case LWS_CALLBACK_CLOSED_CLIENT_HTTP:
		display_g.client_wsi = NULL;
		lws_cancel_service(lws_get_context(wsi));
		break;

	default:
		break;
	}

	return lws_callback_http_dummy(wsi, reason, user, in, len);
}

enum protocols
{
	PROTOCOL_HTTP,
	PROTOCOL_NULL,
	PROTOCOL_COUNT
};

static const struct lws_protocols protocols[PROTOCOL_COUNT] = {
	{
		.name = "http",
		.callback = http_cb,
	},
};

static const cyaml_config_t config = {
		.flags = CYAML_CFG_IGNORE_UNKNOWN_KEYS,
		.log_level = CYAML_LOG_WARNING,
		.mem_fn = cyaml_mem,
		.log_fn = cyaml_log,
};

static void display__free_displays(void)
{
	cyaml_free(&config,
			&display_schema,
			display_g.display,
			display_g.display_count);

	display_g.display = NULL;
	display_g.display_count = 0;
}

static bool display_parse(const uint8_t *data, size_t len)
{
	cyaml_err_t res;

	res = cyaml_load_data(data, len,
			&config,
			&display_schema,
			(void **)&display_g.display,
			&display_g.display_count);
	if (res != CYAML_OK) {
		cdt_log(CDT_LOG_ERROR, "Failed to parse display spec: %s",
				cyaml_strerror(res));
		return false;
	}

	if (display_g.display_count == 0) {
		cdt_log(CDT_LOG_ERROR, "No displays found");
		display__free_displays();
		return false;
	}

	return true;
}

static bool display_init(const char *host, int port)
{
	bool parsed;
	struct lws_context *context;
	struct lws_context_creation_info info = {
		.port = CONTEXT_PORT_NO_LISTEN,
		.protocols = protocols,
		.gid = -1,
		.uid = -1,
	};
	struct lws_client_connect_info i = {
		.port = port,
		.address = host,
		.path = "/json",
		.method = "GET",
		.host = i.address,
		.origin = i.address,
		.protocol = protocols[PROTOCOL_HTTP].name,
		.pwsi = &display_g.client_wsi,
	};

	context = lws_create_context(&info);
	if (context == NULL) {
		cdt_log(CDT_LOG_ERROR, "lws_create_context failed");
		return EXIT_FAILURE;
	}

	i.context = context;
	lws_client_connect_via_info(&i);

	while (display_g.client_wsi && !display_g.interrupted) {
		if (lws_service(context, 1000) < 0) {
			break;
		}
	}

	lws_context_destroy(context);

	if (display_g.multipart_msg.data == NULL) {
		return false;
	}

	parsed = display_parse((const uint8_t *)
			display_g.multipart_msg.data,
			display_g.multipart_msg.len);
	cdt_buffer_delete(&display_g.multipart_msg);
	if (parsed == false) {
		return false;
	}

	return true;
}

static void display_fini(void)
{
	display__free_displays();
}

char *display_get_path(const char *display, const char *host, int port)
{
	char *path = NULL;

	if (strlen(display) > 0 && display[0] == '/') {
		path = strdup(display);

	} else {
		if (!display_init(host, port)) {
			return NULL;
		}

		for (unsigned i = 0; i < display_g.display_count; i++) {
			struct display *d = &display_g.display[i];

			if (strcmp(d->type, "page") != 0) {
				continue;
			}

			if (strstr(d->title, display) != NULL) {
				const char *tmp = strstr(d->debugger_url,
						"/devtools/page");
				if (tmp != NULL) {
					path = strdup(tmp);
				}
				break;
			}
		}

		display_fini();
	}

	return path;
}
