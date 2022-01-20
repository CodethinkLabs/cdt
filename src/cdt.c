/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (c) 2022 Codethink
 */

#include <stdio.h>
#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <libwebsockets.h>

#include "display.h"

#include "cmd/cmd.h"
#include "msg/msg.h"
#include "msg/queue.h"

#include "util/util.h"
#include "util/buffer.h"

struct cdt_ctx {
	struct lws *web_socket;
	bool interrupted;
	void *cmd_pw;

	char *path;
	struct cdt_buffer multipart_msg;
};

static struct cdt_ctx cdt_g;

static bool cdt_send_msg(struct lws *wsi)
{
	char *msg = msg_queue_pop(msg_queue_get_send());
	size_t len;

	if (msg == NULL) {
		return true;
	}

	len = msg_get_len(msg);

	fprintf(stderr, "Sending: %s\n", msg);
	lws_write(wsi, (unsigned char *)msg, len, LWS_WRITE_TEXT);
	msg_queue_push(msg_queue_get_sent(), msg);

	return true;
}

static bool cdt_msg_scan_cb(
		void *pw,
		const struct msg_scan_spec *key,
		const union  msg_scan_data *value)
{
	assert(key != NULL);
	assert(value != NULL);

	CDT_UNUSED(pw);

	if (key->type == MSG_SCAN_TYPE_INTEGER &&
			strcmp(key->key, "id") == 0) {
		int id = (int)value->integer;
		char *msg_sent;

		msg_sent = msg_queue_find_by_id(msg_queue_get_sent(), id);
		if (msg_sent == NULL) {
			fprintf(stderr, "%s: Failed to find sent message: %i\n",
					__func__, id);
		} else {
			msg_queue_remove(msg_queue_get_sent(), msg_sent);
			msg_destroy(msg_sent);
		}

		cmd_msg(cdt_g.cmd_pw, id,
				cdt_g.multipart_msg.data,
				cdt_g.multipart_msg.len);

		return true;

	} else if (key->type == MSG_SCAN_TYPE_STRING &&
			strcmp(key->key, "method") == 0) {
		cmd_evt(cdt_g.cmd_pw,
				value->string.str,
				value->string.len,
				cdt_g.multipart_msg.data,
				cdt_g.multipart_msg.len);
		return true;
	}

	return false;
}

static bool cdt_rec_msg(const char *msg_rec, size_t len)
{
	enum msg_scan scan;
	static const struct msg_scan_spec spec[] = {
		{
			.key = "id",
			.type = MSG_SCAN_TYPE_INTEGER,
			.depth = 1,
		},
		{
			.key = "method",
			.type = MSG_SCAN_TYPE_STRING,
			.depth = 1,
		},
	};

	scan = msg_str_chunk_scan(msg_rec, len);

	switch (scan) {
	case MSG_SCAN_ERROR:
		fprintf(stderr, "%s: Failed to scan message: %*s\n",
				__func__, (int)len, msg_rec);
		cdt_buffer_clear(&cdt_g.multipart_msg);
		return false;

	case MSG_SCAN_COMPLETE:
		if (!cdt_buffer_append(&cdt_g.multipart_msg, msg_rec, len)) {
			return false;
		}

		if (!msg_str_scan(
				cdt_g.multipart_msg.data,
				cdt_g.multipart_msg.len,
				spec, CDT_ARRAY_COUNT(spec),
				cdt_msg_scan_cb, NULL)) {
			fprintf(stderr, "%s: Failed to scan message: %*s\n",
					__func__, (int)len, msg_rec);
		}

		cdt_buffer_clear(&cdt_g.multipart_msg);
		break;

	case MSG_SCAN_CONTINUE:
		if (!cdt_buffer_append(&cdt_g.multipart_msg, msg_rec, len)) {
			return false;
		}
		break;
	}

	return true;
}

static int devtools_cb(struct lws *wsi, enum lws_callback_reasons reason,
		void *user, void *in, size_t len)
{
	(void)(user);

	switch (reason) {
	case LWS_CALLBACK_CLIENT_ESTABLISHED:
		fprintf(stderr, "Connected\n");
		lws_callback_on_writable(wsi);
		break;

	case LWS_CALLBACK_CLIENT_RECEIVE:
		cdt_rec_msg(in, len);
		break;

	case LWS_CALLBACK_CLIENT_WRITEABLE:
		cdt_send_msg(wsi);
		break;

	case LWS_CALLBACK_CLOSED:
	case LWS_CALLBACK_CLIENT_CLOSED:
	case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
		fprintf(stderr, "Disconnected\n");
		cdt_g.web_socket = NULL;
		break;

	default:
		break;
	}

	return 0;
}

enum protocols
{
	PROTOCOL_DEVTOOLS,
	PROTOCOL_NULL,
	PROTOCOL_COUNT
};

static struct lws_protocols protocols[PROTOCOL_COUNT] =
{
	{
		.name = "devtools",
		.callback = devtools_cb,
	},
};

static bool cdt_cli_parse_common(int argc, const char **argv)
{
	enum {
		ARG_CDT,
		ARG_DISPLAY,
		ARG__COUNT,
	};

	if (argc < ARG__COUNT) {
		fprintf(stderr, "Usage:\n");
		fprintf(stderr, "  %s <DISPLAY> <CMD>\n", argv[ARG_CDT]);
		return false;
	}

	cdt_g.path = display_get_path(argv[ARG_DISPLAY]);
	if (cdt_g.path == NULL) {
		fprintf(stderr, "Invalid display: %s\n", argv[ARG_DISPLAY]);
		return false;
	}

	fprintf(stderr, "Using %s as display path\n", cdt_g.path);

	return true;
}

static bool cdt_setup(int argc, const char **argv)
{
	if (!cdt_cli_parse_common(argc, argv)) {
		return false;
	}

	if (!cmd_init(argc, argv, &cdt_g.cmd_pw)) {
		free(cdt_g.path);
		return false;
	}

	return true;
}

static void sigint_handler(int sig)
{
	(void)(sig);
	cdt_g.interrupted = true;
}

static bool cdt_tick_cmd(void *cmd_pw)
{
	if (msg_queue_get_send()->head != NULL) {
		return true;
	}

	return cmd_tick(cmd_pw);
}

static void cdt_run(struct lws_context *context)
{
	struct lws_client_connect_info ccinfo = {
		.port = 9222,
		.context = context,
		.path = cdt_g.path,
		.origin = "origin",
		.address = "localhost",
		.host = lws_canonical_hostname(context),
		.protocol = protocols[PROTOCOL_DEVTOOLS].name,
	};

	cdt_g.web_socket = lws_client_connect_via_info(&ccinfo);

	while (cdt_g.interrupted == false && cdt_g.web_socket != NULL) {
		int ret;
		bool cmd_continue = cdt_tick_cmd(cdt_g.cmd_pw);
		bool need_send = msg_queue_get_send()->head != NULL;
		bool need_resp = msg_queue_get_sent()->head != NULL;

		if (!cmd_continue && !need_send && !need_resp) {
			break;
		}

		lws_callback_on_writable(cdt_g.web_socket);
		ret = lws_service(context, 250);
		if (ret < 0) {
			break;
		}
	}

	cmd_fini(cdt_g.cmd_pw);
	msg_queue_drain(msg_queue_get_send());
	msg_queue_drain(msg_queue_get_sent());
	cdt_buffer_delete(&cdt_g.multipart_msg);
}

int main(int argc, const char *argv[])
{
	struct lws_context *context;
	struct lws_context_creation_info info = {
		.port = CONTEXT_PORT_NO_LISTEN,
		.protocols = protocols,
	};

	signal(SIGINT, sigint_handler);

	lws_set_log_level(LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE,
			lwsl_emit_syslog);

	if (!cdt_setup(argc, argv)) {
		fprintf(stderr, "Setup failed\n");
		return EXIT_FAILURE;
	}

	context = lws_create_context(&info);
	if (context == NULL) {
		fprintf(stderr, "lws_create_context failed\n");
		return EXIT_FAILURE;
	}

	cdt_run(context);
	lws_context_destroy(context);
	free(cdt_g.path);

	return EXIT_SUCCESS;
}
