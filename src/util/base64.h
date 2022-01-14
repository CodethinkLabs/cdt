/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (c) 2022 Codethink
 */

#ifndef CDT_UTIL_BASE64_H
#define CDT_UTIL_BASE64_H

bool base64_decode(const char *b64, size_t b64_len,
		uint8_t **data_out, size_t *len_out);

#endif
