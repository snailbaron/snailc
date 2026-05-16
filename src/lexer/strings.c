#include "strings.h"

#include <stdlib.h>
#include <string.h>

char* str_dup(const char* src)
{
    size_t n = strlen(src) + 1;
    char* dst = malloc(n);
    if (dst != NULL) {
        memcpy(dst, src, n);
    }
    return dst;
}

void str_buffer_clear(str_buffer_t* b)
{
    if (b->ptr != NULL) {
        free(b->ptr);
    }
    *b = (str_buffer_t){0};
}

bool str_buffer_putc(str_buffer_t* b, int c)
{
    if (b->len >= b->cap) {
        size_t new_cap = (2 * b->cap) + 1;
        char* buf = realloc(b->ptr, new_cap);
        if (buf == NULL) {
            fprintf(stderr, "str_buffer_putc: failed to reallocate\n");
            return false;
        }

        b->ptr = buf;
        b->cap = new_cap;
    }

    b->ptr[b->len++] = (char)c;
    return true;
}

char* str_buffer_release(str_buffer_t* b)
{
    char* ptr = b->ptr;
    *b = (str_buffer_t){0};
    return ptr;
}
