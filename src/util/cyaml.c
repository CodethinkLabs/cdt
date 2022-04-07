/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (c) 2022 Codethink
 */

#include <stdio.h>
#include <stdarg.h>

#include "util/log.h"
#include "util/cyaml.h"

void cyaml_log(
		cyaml_log_t level,
		void *ctx,
		const char *fmt,
		va_list args)
{
	static const char * const strings[] = {
		[CYAML_LOG_DEBUG]   = "DEBUG",
		[CYAML_LOG_INFO]    = "INFO",
		[CYAML_LOG_NOTICE]  = "NOTICE",
		[CYAML_LOG_WARNING] = "WARNING",
		[CYAML_LOG_ERROR]   = "ERROR",
	};

	if ((enum cdt_log_level)(ctx) >= CDT_LOG_INFO) {
		fprintf(stderr, "libcyaml: %7.7s: ", strings[level]);
		vfprintf(stderr, fmt, args);
	}
}
