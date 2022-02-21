/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (c) 2022 Codethink
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <syslog.h>

#include "util/log.h"
#include "util/util.h"

static struct log_ctx {
	enum cdt_log_level level;
	enum cdt_log_target target;
} log_ctx = {
	.level  = CDT_LOG_NOTICE,
	.target = CDT_LOG_STDERR,
};

void cdt_log_set_level(enum cdt_log_level level)
{
	log_ctx.level = level;
}

void cdt_log_set_target(enum cdt_log_target target)
{
	log_ctx.target = target;
}

static const char *cdt_log__strlevel(enum cdt_log_level level)
{
	static const char *const log_level_str[] = {
		[CDT_LOG_ERROR]   = "ERROR",
		[CDT_LOG_WARNING] = "WARNING",
		[CDT_LOG_NOTICE]  = "NOTICE",
		[CDT_LOG_INFO]    = "INFO",
		[CDT_LOG_DEBUG]   = "DEBUG",
	};

	if (level >= CDT_ARRAY_COUNT(log_level_str)) {
		level = CDT_LOG_ERROR;
	}

	return log_level_str[level];
}

static int cdt_log__priorety(enum cdt_log_level level)
{
	static const int log_level_str[] = {
		[CDT_LOG_ERROR]   = LOG_ERR,
		[CDT_LOG_WARNING] = LOG_WARNING,
		[CDT_LOG_NOTICE]  = LOG_NOTICE,
		[CDT_LOG_INFO]    = LOG_INFO,
		[CDT_LOG_DEBUG]   = LOG_DEBUG,
	};

	if (level >= CDT_ARRAY_COUNT(log_level_str)) {
		level = CDT_LOG_ERROR;
	}

	return log_level_str[level];
}

void cdt_log(enum cdt_log_level level,
		const char *fmt, ...)
{
	va_list params;

	if (level > log_ctx.level) {
		return;
	}

	va_start(params, fmt);

	if (log_ctx.target == CDT_LOG_SYSLOG) {
		vsyslog(cdt_log__priorety(level), fmt, params);
	} else {
		FILE *target = log_ctx.target == CDT_LOG_STDOUT ?
				stdout : stderr;

		fprintf(target, "cdt: %s: ", cdt_log__strlevel(level));
		vfprintf(target, fmt, params);
		fprintf(target, "\n");
	}

	va_end(params);
}
