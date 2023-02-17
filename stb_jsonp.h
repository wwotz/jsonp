#ifndef JSONP_H_
#define JSONP_H_

/*
 *  To include the implementation in your program,
 *      write: #define JSONP_IMPLEMENTATION
 *  before including the source file in ONE C/C++ source file.
*/

#define JSONP_STATIC static
#define JSONP_EXTERN extern

#define JSONP_VERSION 1

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define JSONP_BUFFER_CAPACITY 256

/* stretchy buffer */
typedef struct {
        char *data;
        int size;
        int capacity;
} buffer_t;

/* json token types */
typedef enum {
        JSONP_EOF = 0,
        JSONP_EMPTY,
        JSONP_OPEN_BRACE,
        JSONP_CLOSE_BRACE,
        JSONP_OPEN_BRACKET,
        JSONP_CLOSE_BRACKET,
        JSONP_COMMA,
        JSONP_COLON,
        JSONP_NUMBER,
        JSONP_STRING,
        JSONP_UNDEFINED,
        JSONP_ERROR,
        JSONP_TYPE_COUNT,
} JSONP_TYPE;

/* structure storing token information */
typedef struct {
        buffer_t token;
        JSONP_TYPE type;
} jsonp_token;

/* json info type enums used to describe the data
   being passed into the jsonp_info structure */
typedef enum {
        JSONP_FILE = 0,
        JSONP_TEXT,
        JSONP_INFO_DATA_COUNT
} JSONP_INFO_DATA_TYPE;

/* structure storing the parser initialisation
   information; @type describes how @data should be
   interpreted:
             JSONP_FILE - @data is a file path
             JSONP_TEXT - @data is the json data
*/
typedef struct {
        JSONP_INFO_DATA_TYPE type;
        const char *data;
} jsonp_info_t;

/* json buffer errors to describe the type of error that occurred within
   the operations on the buffer_t structure */
typedef enum {
        JSONP_NO_BUFFER_ERROR = 0, JSONP_NULL_BUFFER_ERROR,
        JSONP_DATA_BUFFER_ERROR, JSONP_RESIZE_BUFFER_ERROR,
        JSONP_BUFFER_ERRORS_COUNT,
} JSONP_BUFFER_ERRORS;

JSONP_EXTERN jsonp_info_t jsonp_create_json_info(JSONP_INFO_DATA_TYPE type,
                                                 const char *data);

JSONP_EXTERN int jsonp_init_buffer(buffer_t *buffer);
JSONP_EXTERN int jsonp_clear_buffer(buffer_t *buffer);
JSONP_EXTERN int jsonp_append_buffer(buffer_t *buffer, char c);
JSONP_EXTERN int jsonp_write_buffer(buffer_t *buffer, const char *data);
JSONP_EXTERN int jsonp_insert_buffer(buffer_t *buffer, const char *data, int offset);
JSONP_EXTERN int jsonp_resize_buffer(buffer_t *buffer);
JSONP_EXTERN int jsonp_free_buffer(buffer_t *buffer);

JSONP_EXTERN jsonp_token jsonp_peek_token();
JSONP_EXTERN jsonp_token jsonp_get_token();
JSONP_EXTERN jsonp_token jsonp_unget_token(jsonp_token *tok);
JSONP_EXTERN int jsonp_rewind(void);
JSONP_EXTERN int jsonp_set_fd(const char *file);
JSONP_EXTERN void jsonp_close_fd();
JSONP_EXTERN int jsonp_had_error(void);
JSONP_EXTERN const char *jsonp_get_error(void);

#ifdef JSONP_IMPLEMENTATION

/* removed when debugging is being removed */
#define JSONP_DEBUG

#ifdef JSONP_DEBUG

#define JSONP_TOKEN_STACK_CAPACITY 10

/* used in order to un-get tokens */
JSONP_STATIC int jsonp_token_stack_capacity = JSONP_TOKEN_STACK_CAPACITY;
JSONP_STATIC int jsonp_token_stack_size = 0;
JSONP_STATIC int jsonp_token_stack_ptr = 0;
JSONP_STATIC jsonp_token jsonp_token_stack[JSONP_TOKEN_STACK_CAPACITY];

JSONP_STATIC int jp_stack_push(jsonp_token* tok);
JSONP_STATIC jsonp_token *jp_stack_pop();

#define JSONP_DEBUG_STACK_CAPACITY 20

JSONP_STATIC int jsonp_debug_stack_size = 0;
JSONP_STATIC int jsonp_debug_stack_ptr = 0;
JSONP_STATIC char *jsonp_debug_stack[JSONP_DEBUG_STACK_CAPACITY];

JSONP_STATIC int jsonp_push_error_debug(const char *msg);
JSONP_STATIC const char *jsonp_pop_error_debug(void);
JSONP_STATIC int jsonp_stack_full_debug(void);
JSONP_STATIC int jsonp_stack_empty_debug(void);

/* functions to return token primitives */
JSONP_STATIC jsonp_token *jsonp_empty_token();
JSONP_STATIC jsonp_token *jsonp_eof_token();
JSONP_STATIC jsonp_token *jsonp_open_brace_token();
JSONP_STATIC jsonp_token *jsonp_close_brace_token();
JSONP_STATIC jsonp_token *jsonp_open_bracket_token();
JSONP_STATIC jsonp_token *jsonp_close_bracket_token();
JSONP_STATIC jsonp_token *jsonp_string_token();
JSONP_STATIC jsonp_token *jsonp_number_token();
JSONP_STATIC jsonp_token *jsonp_colon_token();
JSONP_STATIC jsonp_token *jsonp_comma_token();
JSONP_STATIC jsonp_token *jsonp_undefined_token();
JSONP_STATIC jsonp_token *jsonp_error_token(const char *msg);

JSONP_STATIC int jsonp_push_error_debug(const char *msg)
{
        if (msg != NULL) {
                if (!jsonp_stack_full_debug())
                        jsonp_debug_stack_size++;
                if (jsonp_debug_stack[jsonp_debug_stack_ptr] != NULL)
                        free(jsonp_debug_stack[jsonp_debug_stack_ptr]);
                if (!(jsonp_debug_stack[jsonp_debug_stack_ptr] = strdup(msg))) {
                        jsonp_debug_stack_ptr--;
                        return -1;
                } else {
                        jsonp_debug_stack_ptr = (jsonp_debug_stack_ptr + 1)
                                % JSONP_DEBUG_STACK_CAPACITY;
                }
        }
        return 0;
}

JSONP_STATIC const char *jsonp_pop_error_debug(void)
{
        if (!jsonp_stack_empty_debug()) {
                jsonp_debug_stack_ptr--;
                if (jsonp_debug_stack_ptr < 0)
                        jsonp_debug_stack_ptr += JSONP_DEBUG_STACK_CAPACITY;
                return jsonp_debug_stack[jsonp_debug_stack_ptr];
        }
}

JSONP_STATIC int jsonp_stack_full_debug(void)
{
        return jsonp_debug_stack_size == JSONP_DEBUG_STACK_CAPACITY;
}

JSONP_STATIC int jsonp_stack_empty_debug(void)
{
        return jsonp_debug_stack_size == 0;
}

#define JP_PUSH_ERROR_DEBUG(msg) jp_push_error_debug(msg);

#else /* !defined(JSONP_DEBUG) */

#define JP_PUSH_ERROR_DEBUG(msg) void(0)

#endif /* JSONP_DEBUG */

JSONP_STATIC jsonp_token *jsonp_empty_token()
{

}

JSONP_STATIC jsonp_token *jsonp_eof_token()
{

}

JSONP_STATIC jsonp_token *jsonp_open_brace_token()
{

}

JSONP_STATIC jsonp_token *jsonp_close_brace_token()
{

}

JSONP_STATIC jsonp_token *jsonp_open_bracket_token()
{

}

JSONP_STATIC jsonp_token *jsonp_close_bracket_token()
{

}

JSONP_STATIC jsonp_token *jsonp_string_token()
{

}

JSONP_STATIC jsonp_token *jsonp_number_token()
{

}

JSONP_STATIC jsonp_token *jsonp_colon_token()
{

}

JSONP_STATIC jsonp_token *jsonp_comma_token()
{

}

JSONP_STATIC jsonp_token *jsonp_undefined_token()
{

}

JSONP_STATIC jsonp_token *jsonp_error_token(const char *msg)
{

}

/* @tok stores the current token
   @lookahead stores the current character in the file or buffer
   @curr_fd stores the file being read
*/
JSONP_STATIC jsonp_token *tok = NULL;
JSONP_STATIC int lookahead;
JSONP_STATIC FILE *curr_fd;

JSONP_EXTERN jsonp_info_t jsonp_create_json_info(JSONP_INFO_DATA_TYPE type,
                                              const char *data)
{
        JSONP_INFO_DATA_TYPE my_type = type;
        const char *my_data = data;
        if (type < 0 || type >= JSONP_INFO_DATA_COUNT)
                type = JSONP_TEXT;

        return (jsonp_info_t) {
                .type = type,
                .data = data
        };
}

JSONP_STATIC const char *jsonp_get_error_buffer(int status)
{
        static const char *msgs[JSONP_BUFFER_ERRORS_COUNT] = {
                "No error",
                "buffer was null!",
                "data was null!",
                "failed to resize buffer!",
        };

        return (status >= 0 && status < JSONP_BUFFER_ERRORS_COUNT
                ? msgs[status] : "Unknown Error");
}

JSONP_EXTERN int jsonp_init_buffer(buffer_t *buffer)
{
        if (buffer == NULL) {
                jsonp_push_error_debug(jsonp_get_error_buffer(JSONP_NULL_BUFFER_ERROR));
                return JSONP_NULL_BUFFER_ERROR;
        }

        if (buffer->data == NULL)
                buffer->data = malloc(sizeof(*buffer->data)
                                      * (JSONP_BUFFER_CAPACITY + 1));

        if (buffer->data == NULL) {
                jsonp_free_buffer(buffer);
                jsonp_push_error_debug(jsonp_get_error_buffer(JSONP_DATA_BUFFER_ERROR));
                return JSONP_DATA_BUFFER_ERROR;
        }

        buffer->size = 0;
        buffer->capacity = JSONP_BUFFER_CAPACITY;
        return jsonp_clear_buffer(buffer);
}

JSONP_EXTERN int jsonp_clear_buffer(buffer_t *buffer)
{
        if (buffer == NULL) {
                jsonp_push_error_debug(jsonp_get_error_buffer(JSONP_NULL_BUFFER_ERROR));
                return JSONP_NULL_BUFFER_ERROR;
        }

        if (buffer->data == NULL) {
                jsonp_push_error_debug(jsonp_get_error_buffer(JSONP_DATA_BUFFER_ERROR));
                return JSONP_DATA_BUFFER_ERROR;
        }

        memset(buffer->data, 0,
               (buffer->capacity + 1) * sizeof(*buffer->data));
        buffer->size = 0;
        return JSONP_NO_BUFFER_ERROR;
}

JSONP_EXTERN int jsonp_append_buffer(buffer_t *buffer, char c)
{
        if (buffer == NULL) {
                jsonp_push_error_debug(jsonp_get_error_buffer(JSONP_NULL_BUFFER_ERROR));
                return JSONP_NULL_BUFFER_ERROR;
        }

        if (buffer->data == NULL) {
                jsonp_push_error_debug(jsonp_get_error_buffer(JSONP_DATA_BUFFER_ERROR));
                return JSONP_DATA_BUFFER_ERROR;
        }

        if (buffer->size >= buffer->capacity) {
                if (jsonp_resize_buffer(buffer) != 0) {
                        jsonp_push_error_debug(jsonp_get_error_buffer(JSONP_RESIZE_BUFFER_ERROR));
                        return JSONP_RESIZE_BUFFER_ERROR;
                }
        }

        buffer->data[buffer->size++] = c;
        buffer->data[buffer->size] = '\0';
        return JSONP_NO_BUFFER_ERROR;
}

JSONP_EXTERN int jsonp_write_buffer(buffer_t *buffer, const char *data)
{
        return jsonp_insert_buffer(buffer, data, 0);
}

JSONP_EXTERN int jsonp_insert_buffer(buffer_t *buffer, const char *data, int offset)
{
        if (buffer == NULL) {
                jsonp_push_error_debug(jsonp_get_error_buffer(JSONP_NULL_BUFFER_ERROR));
                return JSONP_NULL_BUFFER_ERROR;
        }

        if (buffer->data == NULL) {
                jsonp_push_error_debug(jsonp_get_error_buffer(JSONP_DATA_BUFFER_ERROR));
                return JSONP_DATA_BUFFER_ERROR;
        }

        if (data != NULL) {
                int data_len = strlen(data) + offset;
                if (data_len >= buffer->capacity) {
                        int status;
                        if ((status = jsonp_resize_buffer(buffer))
                            != JSONP_NO_BUFFER_ERROR)
                                return status;
                }

                strncpy(buffer->data+offset, data, buffer->capacity);
                buffer->size = data_len;
                buffer->data[data_len] = '\0';
        }

        return JSONP_NO_BUFFER_ERROR;
}

JSONP_EXTERN int jsonp_resize_buffer(buffer_t *buffer)
{
        if (buffer == NULL) {
                jsonp_push_error_debug(jsonp_get_error_buffer(JSONP_NULL_BUFFER_ERROR));
                return JSONP_NULL_BUFFER_ERROR;
        }

        if (buffer->data == NULL) {
                jsonp_push_error_debug(jsonp_get_error_buffer(JSONP_DATA_BUFFER_ERROR));
                return JSONP_DATA_BUFFER_ERROR;
        }

        char *new_data = realloc(buffer->data, (buffer->capacity*2) + 1);
        if (new_data == NULL) {
                jsonp_push_error_debug(jsonp_get_error_buffer(JSONP_RESIZE_BUFFER_ERROR));
                return JSONP_RESIZE_BUFFER_ERROR;
        }

        buffer->data = new_data;
        buffer->capacity *= 2;
        return JSONP_NO_BUFFER_ERROR;
}

JSONP_EXTERN int jsonp_free_buffer(buffer_t *buffer)
{
        if (buffer != NULL) {
                if (buffer->data != NULL) {
                        jsonp_clear_buffer(buffer);
                        free(buffer->data);
                        buffer->data = NULL;
                }

                buffer->size = 0;
                buffer->capacity = 0;
        }
        return JSONP_NO_BUFFER_ERROR;
}

JSONP_EXTERN jsonp_token jsonp_peek_token()
{

}

JSONP_EXTERN jsonp_token jsonp_get_token()
{

}

JSONP_EXTERN jsonp_token jsonp_unget_token(jsonp_token *tok)
{

}

JSONP_EXTERN int jsonp_rewind(void)
{

}

JSONP_EXTERN int jsonp_set_fd(const char *file)
{

}

JSONP_EXTERN void jsonp_close_fd()
{

}

JSONP_EXTERN int jsonp_had_error(void)
{

}

JSONP_EXTERN const char *jsonp_get_error(void)
{

}



#endif /* JSONP_IMPLEMENTATION */

#endif // JSONP_H_
