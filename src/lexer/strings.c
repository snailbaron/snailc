#include "strings.h"

#include <stdlib.h>
#include <string.h>

void str_copy(char** pdest, const char* const src)
{
    if (pdest == NULL) {
        return;
    }

    if (*pdest != NULL) {
        free(*pdest);
        *pdest = NULL;
    }

    if (src == NULL) {
        return;
    }

    size_t n = strlen(src) + 1;
    *pdest = malloc(n);
    strncpy(*pdest, src, n);
}

bool str_read(char** pdest, FILE* stream, long start, size_t len)
{
    if (pdest == NULL) {
        return true;
    }

    if (stream == NULL) {
        (void)fputs("str_read: file stream is NULL", stderr);
    }

    if (*pdest != NULL) {
        free(*pdest);
        *pdest = NULL;
    }

    if (fseek(stream, start, SEEK_SET) != 0) {
        (void)fputs("str_read: failed to fseek to position", stderr);
        return false;
    }
    *pdest = malloc(len + 1);
    if (*pdest == NULL) {
        (void)fputs("str_read: failed to allocate memory for storage", stderr);
        return false;
    }
    (*pdest)[len] = '\0';
    if (fread(*pdest, 1, len, stream) != len) {
        (void)fputs("str_read: failed to read from file", stderr);
        return false;
    }

    return true;
}
