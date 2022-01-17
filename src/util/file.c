/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (c) 2022 Codethink
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>

#include "util/file.h"

void file_write(
		const uint8_t *data, size_t data_len,
		const char *filename_fmt, ...)
{
	char *filename;
	va_list args;
	int ret;
	FILE *f;

	va_start(args, filename_fmt);
	ret = vasprintf(&filename, filename_fmt, args);
	va_end(args);
	if (ret < 0) {
		fprintf(stderr, "%s: Failed to construct screenshot filename\n",
				__func__);
		return;
	}

	f = fopen(filename, "wb");
	if (f == NULL) {
		fprintf(stderr, "%s: Failed to open '%s'\n", __func__,
				filename);
		free(filename);
		return;
	}

	fwrite(data, data_len, 1, f);
	fclose(f);

	fprintf(stderr, "Saved: %s\n", filename);
	free(filename);
}
