/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (c) 2022 Codethink
 */

#ifndef CDT_UTIL_FILE_H
#define CDT_UTIL_FILE_H

void file_write(
		const uint8_t *data, size_t data_len,
		const char *filename_fmt, ...);

#endif
