#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "strings.h"

static int isword(int c)
{
    return isalnum(c) || c == '_';
}

static const char* const keywords[] = {
    "auto", "break", "case", "char", "const", "continue", "default", "do",
    "double", "else", "enum", "extern", "float", "for", "goto", "if", "inline",
    "int", "long", "register", "restrict", "return", "short", "signed",
    "sizeof", "static", "struct", "switch", "typedef", "union", "unsigned",
    "void", "volatile", "while",
};

static bool is_keyword(const char* str)
{
    for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
        if (strcmp(str, keywords[i]) == 0) {
            return true;
        }
    }
    return false;
}

typedef enum {
    TOKEN_TYPE_UNKNOWN,
    TOKEN_TYPE_IDENTIFIER,
    TOKEN_TYPE_CONSTANT,
    TOKEN_TYPE_KEYWORD,
    TOKEN_TYPE_PAREN_OPEN,
    TOKEN_TYPE_PAREN_CLOSE,
    TOKEN_TYPE_BRACE_OPEN,
    TOKEN_TYPE_BRACE_CLOSE,
    TOKEN_TYPE_SEMICOLON,
} token_type_t;

static const char* token_type_string(token_type_t tt)
{
    switch (tt) {
        case TOKEN_TYPE_UNKNOWN: return "unknown";
        case TOKEN_TYPE_IDENTIFIER: return "identifier";
        case TOKEN_TYPE_CONSTANT: return "constant";
        case TOKEN_TYPE_KEYWORD: return "keyword";
        case TOKEN_TYPE_PAREN_OPEN: return "opening parenthesis";
        case TOKEN_TYPE_PAREN_CLOSE: return "closing parenthesis";
        case TOKEN_TYPE_BRACE_OPEN: return "open brace";
        case TOKEN_TYPE_BRACE_CLOSE: return "close brace";
        case TOKEN_TYPE_SEMICOLON: return "semicolon";
    }
    return "bad value";
}

typedef struct {
    token_type_t type;
    char* text;
} token_t;

static void token_clear(token_t* t)
{
    if (t->text != NULL) {
        free(t->text);
        t->text = NULL;
    }
}

static bool parse_whitespace(FILE* in)
{
    for (;;) {
        int c = fgetc(in);
        if (c == EOF) {
            if (ferror(in)) {
                fprintf(stderr, "parse_whitespace: stream error\n");
                return false;
            }
            return true;
        }

        // clang-tidy cannot correctly estimate the range of 'c' here.
        // NOLINTNEXTLINE(clang-analyzer-security.ArrayBound)
        if (!isspace(c)) {
            if (ungetc(c, in) == EOF) {
                fprintf(stderr, "parse_whitespace: ungetc failed after a fgetc!\n");
                return false;
            }
            return true;
        }
    }
}

static bool parse_token(FILE* in, token_t* token)
{
    token_clear(token);

    int c = fgetc(in);
    if (c == EOF) {
        return false;
    }

    if (c == '(') {
        *token = (token_t){.type = TOKEN_TYPE_PAREN_OPEN};
        return true;
    }
    if (c == ')') {
        *token = (token_t){.type = TOKEN_TYPE_PAREN_CLOSE};
        return true;
    }
    if (c == '{') {
        *token = (token_t){.type = TOKEN_TYPE_BRACE_OPEN};
        return true;
    }
    if (c == '}') {
        *token = (token_t){.type = TOKEN_TYPE_BRACE_CLOSE};
        return true;
    }
    if (c == ';') {
        *token = (token_t){.type = TOKEN_TYPE_SEMICOLON};
        return true;
    }

    if (isdigit(c)) {
        str_buffer_t buffer = {0};

        while (isdigit(c)) {
            if (!str_buffer_putc(&buffer, c)) {
                str_buffer_clear(&buffer);
                return false;
            }
            c = fgetc(in);
        }
        if (c != EOF) {
            if (ungetc(c, in) == EOF) {
                str_buffer_clear(&buffer);
                fprintf(stderr, "failed to ungetc\n");
                return false;
            }
        }

        if (!str_buffer_putc(&buffer, '\0')) {
            str_buffer_clear(&buffer);
            return false;
        }

        if (isword(c)) {
            str_buffer_clear(&buffer);
            fprintf(
                stderr,
                "expected a word boundary after constant '%s', got '%c'\n",
                buffer.ptr, c);
            return false;
        }

        token->text = str_buffer_release(&buffer);
        token->type = TOKEN_TYPE_CONSTANT;

        return true;
    }

    if (isalpha(c) || c == '_') {
        str_buffer_t buffer = {0};

        while (isword(c)) {
            if (!str_buffer_putc(&buffer, c)) {
                str_buffer_clear(&buffer);
                return false;
            }
            c = fgetc(in);
        }
        if (c != EOF) {
            if (ungetc(c, in) == EOF) {
                str_buffer_clear(&buffer);
                fprintf(stderr, "failed to ungetc\n");
                return false;
            }
        }

        if (!str_buffer_putc(&buffer, '\0')) {
            str_buffer_clear(&buffer);
            return false;
        }
        token->text = str_buffer_release(&buffer);

        if (is_keyword(token->text)) {
            token->type = TOKEN_TYPE_KEYWORD;
        } else {
            token->type = TOKEN_TYPE_IDENTIFIER;
        }

        return true;
    }

    fprintf(stderr, "unexpected symbol: '%c'\n", c);
    return false;
}

typedef struct {
    size_t len;
    size_t cap;
    token_t* ptr;
} tokens_t;

static void tokens_clear(tokens_t* ts)
{
    if (ts->ptr != NULL) {
        for (size_t i = 0; i < ts->len; i++) {
            token_clear(&ts->ptr[i]);
        }
        free(ts->ptr);
        ts->ptr = NULL;
    }
    ts->len = 0;
    ts->cap = 0;
}

static bool tokens_add(tokens_t* ts, token_t t)
{
    if (ts->len >= ts->cap) {
        ts->cap = (2 * ts->cap) + 1;

        token_t* buf = realloc(ts->ptr, ts->cap * sizeof(token_t));
        if (buf == NULL) {
            fprintf(stderr, "failed to allocate more tokens\n");
            return false;
        }
        ts->ptr = buf;
    }

    ts->ptr[ts->len++] = t;
    return true;
}

static bool tokenize_stream(FILE* in, tokens_t* out)
{
    tokens_clear(out);

    for (;;) {
        if (!parse_whitespace(in)) {
            return false;
        }
        if (feof(in)) {
            break;
        }

        token_t token = {0};
        if (!parse_token(in, &token)) {
            return false;
        }
        if (!tokens_add(out, token)) {
            return false;
        }
    }

    return true;
}

static bool tokenize(const char* file_path, tokens_t* ts)
{
    FILE* file = fopen(file_path, "r");
    if (file == NULL) {
        fprintf(
            stderr, "failed to open file '%s' (errno = %d)\n",
            file_path, errno);
        return false;
    }

    if (!tokenize_stream(file, ts)) {
        fclose(file);
        return false;
    }

    if (fclose(file) == EOF) {
        fprintf(
            stderr, "failed to close file '%s' (errno = %d)\n",
            file_path, errno);
        return false;
    }

    return true;
}

int main(int argc, char* argv[])
{
    if (argc != 2) {
        fprintf(stderr, "usage: lexer FILE\n");
        return EXIT_FAILURE;
    }
    const char* file_path = argv[1];

    tokens_t ts = {0};
    if (!tokenize(file_path, &ts)) {
        return EXIT_FAILURE;
    }

    for (size_t i = 0; i < ts.len; i++) {
        const token_t* t = &ts.ptr[i];
        printf("%s", token_type_string(t->type));
        if (t->text != NULL) {
            printf(" : %s", t->text);
        }
        printf("\n");
    }

    tokens_clear(&ts);
}
