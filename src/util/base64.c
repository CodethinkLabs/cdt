/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (c) 2022 Codethink
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <libwebsockets.h>

#include "util/log.h"
#include "util/base64.h"

bool base64_decode(const char *b64, size_t b64_len,
		uint8_t **data_out, size_t *len_out)
{
	uint8_t *data;
	int ret;

	data = malloc(b64_len);
	if (data == NULL) {
		cdt_log(CDT_LOG_ERROR, "%s: Allocation failed", __func__);
		return false;
	}

	ret = lws_b64_decode_string_len(b64, (int)b64_len,
			(char *)data, (int)b64_len);
	if (ret < 0) {
		cdt_log(CDT_LOG_ERROR, "%s: Base64 decode failed. ret: %i",
				__func__, ret);
		free(data);
		return false;
	}

	*len_out = (size_t)ret;
	*data_out = data;
	return true;
}
