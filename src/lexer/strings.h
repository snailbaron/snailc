#pragma once

#include <stdbool.h>
#include <stdio.h>

char* str_dup(const char* src);

typedef struct {
    char* ptr;
    size_t len;
    size_t cap;
} str_buffer_t;

void str_buffer_clear(str_buffer_t* b);
bool str_buffer_putc(str_buffer_t* b, int c);
char* str_buffer_release(str_buffer_t* b);
