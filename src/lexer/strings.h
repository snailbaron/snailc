#pragma once

#include <stdbool.h>
#include <stdio.h>

void str_copy(char** pdest, const char* src);
bool str_read(char** pdest, FILE* stream, long start, size_t len);
