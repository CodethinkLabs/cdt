/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (c) 2022 Codethink
 */

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "msg.h"
#include "queue.h"
#include "private.h"

struct msg_queue *msg_queue_get_send(void)
{
	return &msg_g.queue_send;
}

struct msg_queue *msg_queue_get_sent(void)
{
	return &msg_g.queue_sent;
}

void msg_queue_push(struct msg_queue *queue, char *msg_str)
{
	struct msg_container *msg = msg_str_to_container(msg_str);

	assert(msg->prev == NULL);
	assert(msg->next == NULL);

	if (queue->head == NULL || queue->tail == NULL) {
		assert(queue->head == NULL);
		assert(queue->tail == NULL);
		queue->head = msg;
		queue->tail = msg;
	} else {
		msg->prev = queue->tail;
		queue->tail->next = msg;
		queue->tail = msg;
	}

	queue->count++;
}

char *msg_queue_pop(struct msg_queue *queue)
{
	struct msg_container *msg = queue->head;

	if (msg == NULL) {
		return NULL;
	}

	queue->head = queue->head->next;
	if (queue->head != NULL) {
		queue->head->prev = NULL;
	}

	if (msg == queue->tail) {
		assert(queue->tail->prev == NULL);
		queue->tail = NULL;
	}

	msg->prev = NULL;
	msg->next = NULL;

	queue->count--;

	return msg->str;
}

char *msg_queue_find_by_id(const struct msg_queue *queue, int id)
{
	struct msg_container *msg = queue->head;

	while (msg != NULL) {
		if (msg->id == id) {
			return msg->str;
		}
		msg = msg->next;
	}

	return NULL;
}

void msg_queue_remove(struct msg_queue *queue, char *msg_str)
{
	struct msg_container *msg = msg_str_to_container(msg_str);

	if (msg == NULL) {
		return;
	}

	if (msg->prev != NULL) {
		msg->prev->next = msg->next;
	}

	if (msg->next != NULL) {
		msg->next->prev = msg->prev;
	}

	if (queue->head == msg) {
		queue->head = msg->next;
	}

	if (queue->tail == msg) {
		queue->tail = msg->prev;
	}

	queue->count--;

	msg->next = NULL;
	msg->prev = NULL;
}

void msg_queue_drain(struct msg_queue *queue)
{
	while (queue->head != NULL) {
		msg_destroy(msg_queue_pop(queue));
	}
}
