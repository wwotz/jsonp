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

#ifdef __cplusplus
extern "C" {
#endif

/* stretchy buffer */
#define JSONP_BUFFER_CAPACITY 256

typedef struct {
        char *data;
        int size;
        int capacity;
} buffer_t;

/* json token types */
typedef enum {
        JSONP_TYPE_EOF = 0,
        JSONP_TYPE_EMPTY,
        JSONP_TYPE_OPEN_BRACE,
        JSONP_TYPE_CLOSE_BRACE,
        JSONP_TYPE_OPEN_BRACKET,
        JSONP_TYPE_CLOSE_BRACKET,
        JSONP_TYPE_COMMA,
        JSONP_TYPE_COLON,
        JSONP_TYPE_NUMBER,
        JSONP_TYPE_STRING,
        JSONP_TYPE_UNDEFINED,
        JSONP_TYPE_ERROR,
        JSONP_TYPE_COUNT,
} JSONP_TYPE;

/* json parser initialisation error codes */
typedef enum {
        JSONP_NO_ERROR = 0,
        JSONP_FILE_ERROR,
        JSONP_BUFFER_ERROR,
        JSONP_ERROR_COUNT,
} JSONP_ERROR;

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

/* create a jsonp info structure */
JSONP_EXTERN jsonp_info_t jsonp_create_json_info(JSONP_INFO_DATA_TYPE type,
                                                 const char *data);
/* initialise the json parpser using the information stored in
   the jsonp_info_t structure */
JSONP_EXTERN int jsonp_init(jsonp_info_t info);

/* free the contents held in the json parser global state,
   this will free the parser's copy of the buffer, and/or close
   the open file it has */
JSONP_EXTERN int jsonp_free(void);

/* operations on the buffer_t structure, returns zero on success,
   otherwise non-zero on error, errors can be queried using a call
   to 'jsonp_get_error()' */
JSONP_EXTERN int jsonp_init_buffer(buffer_t *buffer);
JSONP_EXTERN int jsonp_clear_buffer(buffer_t *buffer);
JSONP_EXTERN int jsonp_append_buffer(buffer_t *buffer, char c);
JSONP_EXTERN int jsonp_write_buffer(buffer_t *buffer, const char *data);
JSONP_EXTERN int jsonp_insert_buffer(buffer_t *buffer, const char *data, int offset);
JSONP_EXTERN int jsonp_resize_buffer(buffer_t *buffer);
JSONP_EXTERN int jsonp_free_buffer(buffer_t *buffer);

/* operations on the jsonp_token structure */
JSONP_EXTERN JSONP_TYPE jsonp_get_type_token(jsonp_token tok);
JSONP_EXTERN const char *jsonp_get_data_token(jsonp_token tok);

/* token operations */
JSONP_EXTERN jsonp_token jsonp_peek_token();
JSONP_EXTERN jsonp_token jsonp_get_token();
JSONP_EXTERN jsonp_token jsonp_unget_token(jsonp_token tok);
JSONP_EXTERN int jsonp_rewind(void);

/* error code operations */
JSONP_EXTERN int jsonp_had_error(void);
JSONP_EXTERN const char *jsonp_get_error(void);


#ifdef __cplusplus
}
#endif

#ifdef JSONP_IMPLEMENTATION

/* removed when debugging is being removed */
#define JSONP_DEBUG

#ifdef JSONP_DEBUG

#define JSONP_TOKEN_STACK_CAPACITY 10

/* @tok stores the current token
   @lookahead stores the current character in the file or buffer
   @curr_fd stores the file being read
*/
JSONP_STATIC jsonp_token tok;
JSONP_STATIC int lookahead;
JSONP_STATIC FILE *curr_fd;
JSONP_STATIC buffer_t curr_buffer;
JSONP_STATIC int curr_buffer_ptr;
JSONP_STATIC int (* next_char)(void);

/* used in order to un-get tokens, and peek tokens */
JSONP_STATIC int jsonp_token_stack_size = 0;
JSONP_STATIC int jsonp_token_stack_ptr = 0;
JSONP_STATIC jsonp_token jsonp_token_stack[JSONP_TOKEN_STACK_CAPACITY];

JSONP_STATIC int jsonp_push_token_stack(jsonp_token tok);
JSONP_STATIC jsonp_token jsonp_pop_token_stack(void);
JSONP_STATIC int jsonp_empty_token_stack(void);
JSONP_STATIC int jsonp_full_token_stack(void);

#define JSONP_DEBUG_STACK_CAPACITY 20

JSONP_STATIC int jsonp_debug_stack_size = 0;
JSONP_STATIC int jsonp_debug_stack_ptr = 0;
JSONP_STATIC char *jsonp_debug_stack[JSONP_DEBUG_STACK_CAPACITY];

JSONP_STATIC int jsonp_push_error_debug(const char *msg);
JSONP_STATIC const char *jsonp_pop_error_debug(void);
JSONP_STATIC int jsonp_stack_full_debug(void);
JSONP_STATIC int jsonp_stack_empty_debug(void);

/* functions to return token primitives */
JSONP_STATIC jsonp_token jsonp_empty_token();
JSONP_STATIC jsonp_token jsonp_eof_token();
JSONP_STATIC jsonp_token jsonp_open_brace_token();
JSONP_STATIC jsonp_token jsonp_close_brace_token();
JSONP_STATIC jsonp_token jsonp_open_bracket_token();
JSONP_STATIC jsonp_token jsonp_close_bracket_token();
JSONP_STATIC jsonp_token jsonp_string_token();
JSONP_STATIC jsonp_token jsonp_number_token();
JSONP_STATIC jsonp_token jsonp_colon_token();
JSONP_STATIC jsonp_token jsonp_comma_token();
JSONP_STATIC jsonp_token jsonp_undefined_token();
JSONP_STATIC jsonp_token jsonp_error_token(const char *msg);

JSONP_STATIC int jsonp_push_token_stack(jsonp_token tok)
{
        if (!jsonp_full_token_stack())
                jsonp_token_stack_size++;
        jsonp_token_stack[jsonp_token_stack_ptr].type = tok.type;

        int status;
        if ((status = jsonp_write_buffer(&jsonp_token_stack[jsonp_token_stack_ptr].token, tok.token.data)) != JSONP_NO_BUFFER_ERROR)
                return status;
        jsonp_token_stack_ptr = (jsonp_token_stack_ptr + 1) % JSONP_TOKEN_STACK_CAPACITY;
        return JSONP_NO_ERROR;
}

JSONP_STATIC jsonp_token jsonp_pop_token_stack(void)
{
        if (jsonp_empty_token_stack())
                return jsonp_empty_token();

        jsonp_token_stack_ptr--;
        jsonp_token_stack_size--;
        if (jsonp_token_stack_ptr < 0)
                jsonp_token_stack_ptr += JSONP_TOKEN_STACK_CAPACITY;

        tok.type = jsonp_token_stack[jsonp_token_stack_ptr].type;
        jsonp_write_buffer(&tok.token, jsonp_token_stack[jsonp_token_stack_ptr].token.data);
        return tok;
}

JSONP_STATIC int jsonp_empty_token_stack(void)
{
        return jsonp_token_stack_size == 0;
}

JSONP_STATIC int jsonp_full_token_stack(void)
{
        return jsonp_token_stack_size == JSONP_TOKEN_STACK_CAPACITY;
}

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

        return "No Error";
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

JSONP_STATIC int next_char_file(void)
{
        return fgetc(curr_fd);
}

JSONP_STATIC int next_char_buffer(void)
{
        if (curr_buffer_ptr < curr_buffer.size)
                return curr_buffer.data[curr_buffer_ptr++];
        return EOF;
}

JSONP_STATIC jsonp_token jsonp_empty_token()
{
        memset(&tok, 0, sizeof(tok));
        if (jsonp_init_buffer(&tok.token) != JSONP_NO_BUFFER_ERROR)
                return tok;

        tok.type = JSONP_TYPE_EMPTY;
        jsonp_clear_buffer(&tok.token);
        return tok;
}

JSONP_STATIC jsonp_token jsonp_eof_token()
{
        tok = jsonp_empty_token();
        tok.type = JSONP_TYPE_EOF;
        jsonp_write_buffer(&tok.token, "EOF");
        lookahead = next_char();
        return tok;
}

JSONP_STATIC jsonp_token jsonp_open_brace_token()
{
        tok = jsonp_empty_token();
        tok.type = JSONP_TYPE_OPEN_BRACE;
        jsonp_write_buffer(&tok.token, "{");
        lookahead = next_char();
        return tok;
}

JSONP_STATIC jsonp_token jsonp_close_brace_token()
{
        tok = jsonp_empty_token();
        tok.type = JSONP_TYPE_CLOSE_BRACE;
        jsonp_write_buffer(&tok.token, "}");
        lookahead = next_char();
        return tok;
}

JSONP_STATIC jsonp_token jsonp_open_bracket_token()
{
        tok = jsonp_empty_token();
        tok.type = JSONP_TYPE_OPEN_BRACKET;
        jsonp_write_buffer(&tok.token, "[");
        lookahead = next_char();
        return tok;
}

JSONP_STATIC jsonp_token jsonp_close_bracket_token()
{
        tok = jsonp_empty_token();
        tok.type = JSONP_TYPE_CLOSE_BRACKET;
        jsonp_write_buffer(&tok.token, "]");
        lookahead = next_char();
        return tok;
}

JSONP_STATIC jsonp_token jsonp_string_token()
{
        tok = jsonp_empty_token();
        tok.type = JSONP_TYPE_STRING;
        lookahead = next_char();
        while (lookahead != '"' && lookahead != EOF) {
                jsonp_append_buffer(&tok.token, lookahead);
                lookahead = next_char();
        }

        if (lookahead == EOF) {
                tok = jsonp_error_token("Unterminated string");
        } else {
                lookahead = next_char();
        }
        return tok;
}

JSONP_STATIC jsonp_token jsonp_number_token()
{
        tok = jsonp_empty_token();
        tok.type = JSONP_TYPE_NUMBER;
        while ((lookahead >= '0' && lookahead <= '9')) {
                jsonp_append_buffer(&tok.token, lookahead);
                lookahead = next_char();
        }

        if (lookahead == '.') {
                jsonp_append_buffer(&tok.token, lookahead);
                lookahead = next_char();
                while ((lookahead >= '0' && lookahead <= '9')) {
                        jsonp_append_buffer(&tok.token, lookahead);
                        lookahead = next_char();
                }
        }
        return tok;
}

JSONP_STATIC jsonp_token jsonp_colon_token()
{
        tok = jsonp_empty_token();
        tok.type = JSONP_TYPE_COLON;
        jsonp_write_buffer(&tok.token, ":");
        lookahead = next_char();
        return tok;
}

JSONP_STATIC jsonp_token jsonp_comma_token()
{
        tok = jsonp_empty_token();
        tok.type = JSONP_TYPE_COMMA;
        jsonp_write_buffer(&tok.token, ",");
        lookahead = next_char();
        return tok;
}

JSONP_STATIC jsonp_token jsonp_undefined_token()
{
        tok = jsonp_empty_token();
        tok.type = JSONP_TYPE_UNDEFINED;
        jsonp_write_buffer(&tok.token, "UNDEFINED");
        lookahead = next_char();
        return tok;
}

JSONP_STATIC jsonp_token jsonp_error_token(const char *msg)
{
        tok = jsonp_empty_token();
        tok.type = JSONP_TYPE_ERROR;
        if (msg != NULL)
                jsonp_write_buffer(&tok.token, msg);
        else
                jsonp_write_buffer(&tok.token, "Error: no description");
        return tok;
}

JSONP_STATIC const char *jsonp_get_error_init(int status)
{
        static const char *msgs[JSONP_ERROR_COUNT] = {
                "No Error",
                "File does not exist!",
                "Could not create buffer!",
        };

        return (status >= 0 && status < JSONP_ERROR_COUNT
            ? msgs[status] : "Undefined Error!");
}

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

JSONP_EXTERN int jsonp_init(jsonp_info_t info)
{
        tok = jsonp_empty_token();
        curr_buffer_ptr = 0;

        switch (info.type) {
        case JSONP_FILE:
                next_char = next_char_file;
                curr_fd = fopen(info.data, "r");
                if (!curr_fd) {
                        jsonp_push_error_debug(jsonp_get_error_init(JSONP_FILE_ERROR));
                        return JSONP_FILE_ERROR;
                }
                break;
        case JSONP_TEXT:
        default: /* JSONP_TEXT is also the default case */
                next_char = next_char_buffer;
                jsonp_init_buffer(&curr_buffer);
                jsonp_write_buffer(&curr_buffer, info.data);
                if (jsonp_had_error()) {
                        jsonp_push_error_debug(jsonp_get_error_init(JSONP_BUFFER_ERROR));
                        return JSONP_BUFFER_ERROR;
                }
                break;
        }

        lookahead = next_char();
        return JSONP_NO_ERROR;
}

JSONP_EXTERN int jsonp_free(void)
{
        if (curr_fd) {
                fclose(curr_fd);
                curr_fd = NULL;
        }

        jsonp_free_buffer(&curr_buffer);

        return JSONP_NO_ERROR;
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
                buffer->data = (typeof(buffer->data))malloc(sizeof(*buffer->data)
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

        char *new_data = (typeof(new_data))realloc(buffer->data, (buffer->capacity*2) + 1);
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

JSONP_EXTERN JSONP_TYPE jsonp_get_type_token(jsonp_token tok)
{
        return tok.type;
}

JSONP_EXTERN const char *jsonp_get_data_token(jsonp_token tok)
{
        return tok.token.data;
}

JSONP_EXTERN jsonp_token jsonp_peek_token()
{
        jsonp_push_token_stack(jsonp_get_token());
        return tok;
}

JSONP_EXTERN jsonp_token jsonp_get_token()
{
        if (!jsonp_empty_token_stack())
                return jsonp_pop_token_stack();

        while (lookahead == ' ' || lookahead == '\t'
               || lookahead == '\n' || lookahead == '\r')
                lookahead = next_char();

        switch (lookahead) {
        case EOF:
                return jsonp_eof_token();
        case '{':
                return jsonp_open_brace_token();
        case '}':
                return jsonp_close_brace_token();
        case '[':
                return jsonp_open_bracket_token();
        case ']':
                return jsonp_close_bracket_token();
        case ',':
                return jsonp_comma_token();
        case ':':
                return jsonp_colon_token();
        case '"':
                return jsonp_string_token();
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
                return jsonp_number_token();
        default:
                return jsonp_undefined_token();
        }
}

JSONP_EXTERN jsonp_token jsonp_unget_token(jsonp_token tok)
{
        jsonp_push_token_stack(tok);
        return tok;
}

JSONP_EXTERN int jsonp_rewind(void)
{
        curr_buffer_ptr = 0;
        return fseek(curr_fd, 0, SEEK_SET);
}

JSONP_EXTERN int jsonp_had_error(void)
{
        return !jsonp_stack_empty_debug();
}

JSONP_EXTERN const char *jsonp_get_error(void)
{
        return jsonp_pop_error_debug();
}

#endif /* JSONP_IMPLEMENTATION */

#endif // JSONP_H_
