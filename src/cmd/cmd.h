/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (c) 2022 Codethink
 */

#ifndef CDT_CMD_H
#define CDT_CMD_H

/**
 * \file
 * \brief Command interface.
 *
 * `cdt` supports multiple subcommands. Each subcommand (cmd) is implemented
 * as a callback table. These callbacks are called at key points through
 * the program lifecycle.
 */

/**
 * Initialise the command.
 *
 * Detects which subcommand is requested, reads and CLI arguments it needs,
 * and initialises the cmd-specific code.
 *
 * \param[in]  argc    Number of command line arguments.
 * \param[in]  argv    String vector containing command line arguments.
 * \param[out] pw_out  Returns private cmd data to be passed to other calls.
 * \return true on success, false otherwise.
 */
bool cmd_init(int argc, const char **argv, void **pw_out);

/**
 * Let the command handle a received message.
 *
 * \param[in] pw   The command's private context.
 * \param[in] id   Id of dent message that this is a response to.
 * \param[in] msg  Received message.
 * \param[in] len  Length of msg in bytes.
 */
void cmd_msg(void *pw, int id, const char *msg, size_t len);

/**
 * .Tick the command.
 *
 * \param[in] pw  The command's private context.
 * \return true if the mainloop should continue even when message queues are
 *         empty, or false to allow termination of the program.
 */
bool cmd_tick(void *pw);

/**
 * .Finalise the command.
 *
 * \param[in] pw  The command's private context.
 */
void cmd_fini(void *pw);

#endif
