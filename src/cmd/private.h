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

	void (*help)(int argc, const char **argv);
	bool (*init)(int argc, const char **argv, void **pw_out);
	void (*msg) (void *pw, int id, const char *msg, size_t len);
	void (*evt) (void *pw, const char *method, size_t method_len,
			const char *msg, size_t len);
	bool (*tick)(void *pw);
	void (*fini)(void *pw);
};

#endif
