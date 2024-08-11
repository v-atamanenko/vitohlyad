/*
 * Copyright (C) 2024 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include "utils.h"

#include "sha1.h"

#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/clib.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void str_replace(const char * src, char * dst, const char * needle, const char * replacement) {
    if (!src)
        return;

    if (!dst)
        return;

    size_t needle_len = strlen(needle);
    size_t repl_len = strlen(replacement);
    size_t target_len = strlen(src);

    if (needle_len == 0 || target_len == 0) {
        return;
    }

    unsigned int count = 0;
    const char *pp = strstr(src, needle);
    while (pp != NULL) {
        count++;
        pp = strstr(pp + needle_len, needle);
    }

    if (count == 0) {
        return;
    }

    size_t final_len = target_len + (repl_len * count) - (needle_len * count);

    char * insert_point = &dst[0];
    const char *tmp = src;

    while (1) {
        const char *p = strstr(tmp, needle);

        if (p == NULL) {
            strcpy(insert_point, tmp);
            break;
        }

        memcpy(insert_point, tmp, p - tmp);
        insert_point += p - tmp;

        memcpy(insert_point, replacement, repl_len);
        insert_point += repl_len;

        tmp = p + needle_len;
    }

    dst[final_len] = '\0';
}

char * str_sha1sum(const char * str, size_t size) {
    if (size == 0) {
        size = strlen(str);
    }

    uint8_t sha1[20];
    SHA1_CTX ctx;
    sha1_init(&ctx);
    sha1_update(&ctx, (uint8_t *)str, size);
    sha1_final(&ctx, (uint8_t *)sha1);

    char hash[42];
    memset(hash, 0, sizeof(hash));

    for (int i = 0; i < 20; i++) {
        char string[4];
        sprintf(string, "%02x", sha1[i]);
        strcat(hash, string);
    }

    hash[41] = '\0';
    return strdup(hash);
}

char * file_sha1sum(const char *path) {
    char * buffer;

    SceUID fd = sceIoOpen(path, SCE_O_RDONLY, 0777);
    if (fd < 0) {
        sceClibPrintf("file_sha1sum: sceIoOpen FAILED for path %s\n", path);
        return NULL;
    }

    SceIoStat stat;
    if (sceIoGetstatByFd(fd, &stat) < 0) {
        sceIoClose(fd);
        sceClibPrintf("file_sha1sum: sceIoGetStat FAILED for path %s\n", path);
        return NULL;
    }

    SceOff file_sz = stat.st_size;

    buffer = malloc(file_sz);
    if (!buffer) {
        sceClibPrintf("file_sha1sum: malloc FAILED for path %s (size %u)\n", path, file_sz);
        sceIoClose(fd);
        return NULL;
    }

    SceSSize bytes_read_total = 0;
    SceSSize bytes_read;
    do {
        bytes_read = sceIoRead(fd, buffer + bytes_read_total, 64 * 1024);
        bytes_read_total += bytes_read;
    } while (bytes_read > 0);

    char * ret = str_sha1sum((const char *) buffer, file_sz);

    free(buffer);
    sceIoClose(fd);
    return ret;
}

bool sha1sum_file_check(const char *path, const char* reference) {
    bool ret = false;
    char * filesum = file_sha1sum(path);
    if (filesum) {
        if (strcmp(filesum, reference) == 0) {
            ret = true;
        }
        free(filesum);
    }
    return ret;
}
