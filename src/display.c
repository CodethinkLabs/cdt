/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (c) 2022 Codethink
 */

#include <string.h>

#include "display.h"

char *display_get_path(const char *display)
{
	char *path = NULL;

	if (strlen(display) > 0 && display[0] == '/') {
		path = strdup(display);

	}

	return path;
}
