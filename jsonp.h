#ifndef JSONP_H_
#define JSONP_H_

#include <stdlib.h>
#include <string.h>

#define BUFFER_T_CAPACITY (256)

struct buffer_t {
        char *data;
        int size;
        int capacity;
};
typedef struct buffer_t buffer_t;

int init_buffer_t(buffer_t *buffer);
int append_buffer_t(buffer_t *buffer, char c);
int write_buffer_t(buffer_t *buffer, const char *data);
int resize_buffer_t(buffer_t *buffer);
int free_buffer_t(buffer_t *buffer);

#endif // JSONP_H_
