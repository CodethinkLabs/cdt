/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (c) 2022 Codethink
 */

#ifndef CDT_UTIL_LOG_H
#define CDT_UTIL_LOG_H

enum cdt_log_level {
	CDT_LOG_ERROR,
	CDT_LOG_WARNING,
	CDT_LOG_NOTICE,
	CDT_LOG_INFO,
	CDT_LOG_DEBUG,
};

enum cdt_log_target {
	CDT_LOG_STDERR,
	CDT_LOG_STDOUT,
	CDT_LOG_SYSLOG,
};

/**
 * Set the minimum log level to record.
 *
 * Any messages more trivial than this level will be dropped.
 *
 * \param[in] level  Minimum level to record.
 */
void cdt_log_set_level(enum cdt_log_level level);

/**
 * Get the minimum log level to record.
 *
 * \return Minimum level to record.
 */
enum cdt_log_level cdt_log_get_level(void);

/**
 * Set the log target.
 *
 * \param[in] target  Where to send the log messages.
 */
void cdt_log_set_target(enum cdt_log_target target);

/**
 * Record a message to the log.
 *
 * \param[in] level  The severity of the message.
 * \param[in] fmt    The format string for the message to be logged.
 * \param[in] ...    Variadic arguments.
 */
void cdt_log(enum cdt_log_level level,
		const char *fmt, ...);

#endif
