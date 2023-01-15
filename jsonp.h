#ifndef JSONP_H_
#define JSONP_H_

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define BUFFER_T_CAPACITY (256)

struct buffer_t {
        char *data;
        int size;
        int capacity;
};
typedef struct buffer_t buffer_t;

struct json_token {
        buffer_t token;
        int type;
};
typedef struct json_token json_token;

enum JSON_TYPE {
        JSON_EOF = 0,
        JSON_EMPTY,
        JSON_OPEN_BRACE,
        JSON_CLOSE_BRACE,
        JSON_OPEN_BRACKET,
        JSON_CLOSE_BRACKET,
        JSON_COMMA,
        JSON_COLON,
        JSON_NUMBER,
        JSON_STRING,
        JSON_UNDEFINED,
        JSON_ERROR,
        JSON_TYPE_COUNT,
};

int init_buffer_t(buffer_t *buffer);
int clear_buffer_t(buffer_t *buffer);
int append_buffer_t(buffer_t *buffer, char c);
int write_buffer_t(buffer_t *buffer, const char *data);
int insert_buffer_t(buffer_t *buffer, const char *data, int offset);
int resize_buffer_t(buffer_t *buffer);
int free_buffer_t(buffer_t *buffer);

struct json_token *jp_peek_token();
struct json_token *jp_get_token();
struct json_token *jp_unget_token(struct json_token *tok);
int jp_rewind();
int jp_set_fd(FILE *fd);

#endif // JSONP_H_
