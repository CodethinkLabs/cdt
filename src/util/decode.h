/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (c) 2022 Codethink
 */

#ifndef CDT_UTIL_DECODE_H
#define CDT_UTIL_DECODE_H

/**
 * Extract the value from a response.
 *
 * \param[in]  msg  The message to extract a value from.
 * \param[in]  len  The length of the message to extract.
 * \return String of the value, owned by caller, or NULL on error.
 */
char *decode_extract_response_value(const char *msg, size_t len);

/**
 * Extract the integer value from a response.
 *
 * \param[in]  msg  The message to extract a value from.
 * \param[in]  len  The length of the message to extract.
 * \param[out] ret  Returns an integer on success.
 * \return Returns true on success of false on error.
 */
bool decode_extract_response_value_int(const char *msg, size_t len, int *ret);

#endif
