/*
 * Copyright (C) 2024 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#ifndef VITOHLYAD_UTILS_H
#define VITOHLYAD_UTILS_H

#include <stdbool.h>
#include <stddef.h>

char * str_sha1sum(const char * str, size_t size);
void str_replace(const char * src, char * dst, const char * needle, const char * replacement);

char * file_sha1sum(const char *path);
bool sha1sum_file_check(const char *path, const char* reference);

#endif //VITOHLYAD_UTILS_H
