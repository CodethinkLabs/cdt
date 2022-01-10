/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (c) 2022 Codethink
 */

#ifndef CDT_MSG_QUEUE_H
#define CDT_MSG_QUEUE_H

struct msg_container;

struct msg_queue {
	struct msg_container *head;
	struct msg_container *tail;
	unsigned count;
};

struct msg_queue *msg_queue_get_send(void);

struct msg_queue *msg_queue_get_sent(void);

void msg_queue_push(struct msg_queue *queue, char *msg_str);

char *msg_queue_pop(struct msg_queue *queue);

char *msg_queue_find_by_id(const struct msg_queue *queue, int id);

void msg_queue_remove(struct msg_queue *queue, char *msg_str);

void msg_queue_drain(struct msg_queue *queue);

#endif
