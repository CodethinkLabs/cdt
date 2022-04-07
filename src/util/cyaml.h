/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (c) 2022 Codethink
 */

#ifndef CDT_UTIL_CYAML_H
#define CDT_UTIL_CYAML_H

#include <cyaml/cyaml.h>

/**
 * `cdt` CYAML logging function.
 *
 * \param[in] level  Log level of message to log.
 * \param[in] ctx    Logging context, unused.
 * \param[in] fmt    Format string for message to log.
 * \param[in] args   Additional arguments used by fmt.
 */
void cdt_cyaml_log(
		cyaml_log_t level,
		void *ctx,
		const char *fmt,
		va_list args);

#endif
