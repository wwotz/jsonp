/* jsonp.c - json parser written in C */
#include "jsonp.h"

enum JSON_TYPE {
        JSON_EOF = 0,
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

struct json_token {
        buffer_t token;
        int type;
};

int init_buffer_t(buffer_t *buffer)
{
        if (buffer == NULL) return -1;
        buffer->data = malloc(sizeof(*buffer->data) * (BUFFER_T_CAPACITY + 1));
        if (buffer->data == NULL) {
                free_buffer_t(buffer);
                return -1;
        }
        buffer->size = 0;
        buffer->capacity = BUFFER_T_CAPACITY;
        return 0;
}

int append_buffer_t(buffer_t *buffer, char c)
{
        if (buffer == NULL) return -1;
        if (buffer->size >= buffer->capacity) {
                if (resize_buffer_t(buffer) != 0)
                        return -1;
        }
        buffer->data[buffer->size++] = c;
        buffer->data[buffer->size] = '\0';
        return 0;
}

int resize_buffer_t(buffer_t *buffer)
{
        char *new_data = realloc(buffer->data, buffer->capacity*2);
        if (new_data == NULL) return -1;
        buffer->data = new_data;
        buffer->capacity *= 2;
        return 0;
}

int write_buffer_t(buffer_t *buffer, const char *data)
{
        if (buffer == NULL || data == NULL) return -1;
        int data_len = strlen(data);
        if (data_len >= buffer->capacity) {
                if (resize_buffer_t(buffer) != 0)
                        return -1;
        }
        strncpy(buffer->data, data, buffer->capacity);
        buffer->size = data_len;
        buffer->data[data_len] = '\0';
        return 0;
}

int free_buffer_t(buffer_t *buffer)
{
        if (buffer == NULL) return -1;
        if (buffer->data != NULL) {
                free(buffer->data);
                buffer->data = NULL;
        }

        buffer->size = 0;
        buffer->capacity = 0;
        return 0;
}
