/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (c) 2022 Codethink
 */

#ifndef CDT_CMD_PRIVATE_H
#define CDT_CMD_PRIVATE_H

/**
 * Command implementation vtable.
 */
struct cmd_table {
	const char *cmd;

	bool (*init)(int argc, const char **argv, void **pw_out);
	void (*msg) (void *pw, int id, const char *msg, size_t len);
	bool (*tick)(void *pw);
	void (*fini)(void *pw);
};

#endif
