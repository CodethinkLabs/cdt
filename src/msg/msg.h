/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (c) 2022 Codethink
 */

#ifndef CDT_MSG_H
#define CDT_MSG_H

struct msg {
	enum msg_type {
		MSG_TYPE_TOUCH_EVENT_START,
		MSG_TYPE_TOUCH_EVENT_END,
	} type;

	union {
		struct {
			int x;
			int y;
		} touch_event;
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

bool msg_str_id(const char *str, size_t len, int *id);

#endif
