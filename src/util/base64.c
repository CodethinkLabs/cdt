/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (c) 2022 Codethink
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <libwebsockets.h>

#include "util/base64.h"

bool base64_decode(const char *b64, size_t b64_len,
		uint8_t **data_out, size_t *len_out)
{
	uint8_t *data;
	int ret;

	data = malloc(b64_len);
	if (data == NULL) {
		fprintf(stderr, "%s: Allocation failed\n", __func__);
		return false;
	}

	ret = lws_b64_decode_string_len(b64, (int)b64_len,
			(char *)data, (int)b64_len);
	if (ret < 0) {
		fprintf(stderr, "%s: Base64 decode failed. ret: %i\n",
				__func__, ret);
		free(data);
		return false;
	}

	*len_out = (size_t)ret;
	*data_out = data;
	return true;
}
