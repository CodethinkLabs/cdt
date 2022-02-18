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

/** Common parameters. */
struct cmd_options {
	const char *display;
};

/**
 * Print help text for the command.
 *
 * \param[in]  argc    Number of command line arguments.
 * \param[in]  argv    String vector containing command line arguments.
 * \param[in]  cmd     Command to get help for, or NULL to use argument.
 */
void cmd_help(int argc, const char **argv, const char *cmd);

/**
 * Initialise the command.
 *
 * Detects which subcommand is requested, reads and CLI arguments it needs,
 * and initialises the cmd-specific code.
 *
 * \param[in]  argc     Number of command line arguments.
 * \param[in]  argv     String vector containing command line arguments.
 * \param[in]  options  Common command options parsed from arguments.
 * \param[out] pw_out   Returns private cmd data to be passed to other calls.
 * \return true on success, false otherwise.
 */
bool cmd_init(int argc, const char **argv,
		struct cmd_options *options, void **pw_out);

/**
 * Let the command handle a received message.
 *
 * \param[in] pw   The command's private context.
 * \param[in] id   Id of message that this is a response to.
 * \param[in] msg  Received message.
 * \param[in] len  Length of msg in bytes.
 */
void cmd_msg(void *pw, int id, const char *msg, size_t len);

/**
 * Let the command handle a received message.
 *
 * \param[in] pw          The command's private context.
 * \param[in] method      The event method name.
 * \param[in] method_len  Length of method in bytes.
 * \param[in] msg         Received message.
 * \param[in] len         Length of msg in bytes.
 */
void cmd_evt(void *pw, const char *method, size_t method_len,
		const char *msg, size_t len);

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

/**
 * Print a list of available commands.
 */
void cmd_print_command_list(void);

#endif
