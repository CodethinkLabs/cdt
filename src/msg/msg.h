/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (c) 2022 Codethink
 */

#ifndef CDT_MSG_H
#define CDT_MSG_H

struct msg {
	enum msg_type {
		MSG_TYPE_SCREENCAST_FRAME_ACK,
		MSG_TYPE_CAPTURE_SCREENSHOT,
		MSG_TYPE_START_SCREENCAST,
		MSG_TYPE_STOP_SCREENCAST,
		MSG_TYPE_SCROLL_GESTURE,
		MSG_TYPE_TOUCH_EVENT_START,
		MSG_TYPE_TOUCH_EVENT_MOVE,
		MSG_TYPE_TOUCH_EVENT_END,
		MSG_TYPE_EVALUATE,
	} type;

	union {
		struct {
			/** JSON escaped JavaScript expression to run. */
			const char *expression;
		} evaluate;
		struct {
			int x;
			int y;
		} touch_event;
		struct {
			int x;
			int y;
			int speed;
			int x_dist;
			int y_dist;
		} scroll_gesture;
		struct {
			const char *format;
		} capture_screenshot;
		struct {
			/* If max width or height are zero the resolution
			 * will be unconstrained. */
			int max_width;
			int max_height;
			const char *format;
		} start_screencast;
		struct {
			int session_id;
		} screencast_frame_ack;
	} data;
};

void msg_destroy(char *msg);

size_t msg_get_len(char *msg_str);

bool msg_to_msg_str(const struct msg *msg, char **msg_str, int *id_out);

bool msg_queue_for_send(const struct msg *msg, int *id_out);

enum msg_scan {
	MSG_SCAN_COMPLETE, /**< Have complete message. */
	MSG_SCAN_CONTINUE, /**< More date required. */
	MSG_SCAN_ERROR,    /**< Error detected. */
};

/**
 * Scan a message string (JSON) for an ID at the top level.
 *
 * \param[in]  str  Message chunk data.
 * \param[in]  len  Message chunk length.
 * \param[out] id   Returns message ID on MSG_SCAN_COMPLETE only.
 * \return MSG_SCAN_COMPLETE if message was parsed completely.
 *         MSG_SCAN_CONTINUE if message needs more data.
 *         MSG_SCAN_ERROR on error.
 */
enum msg_scan msg_str_chunk_scan(const char *str, size_t len);

struct msg_scan_spec {
	int depth;
	const char *key;
	enum msg_scan_type {
		MSG_SCAN_TYPE_FLOATING_POINT,
		MSG_SCAN_TYPE_INTEGER,
		MSG_SCAN_TYPE_STRING,
	} type;
};
union msg_scan_data {
	struct {
		const char *str;
		size_t len;
	} string;
	int64_t integer;
	double floating_point;
};

typedef bool (*msg_scan_cb)(void *pw,
		const struct msg_scan_spec *key,
		const union  msg_scan_data *value);

bool msg_str_scan(const char *str, size_t len,
		const struct msg_scan_spec *spec, unsigned spec_count,
		msg_scan_cb cb, void *pw);

#endif
