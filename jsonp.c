/* jsonp.c - json parser written in C */
#include "jsonp.h"

/* pointer to the current token inside the file */
static struct json_token *tok = NULL;
static int lookahead;
static FILE *curr_fd;

#define JP_TOKEN_STACK_CAPACITY (10)

/* used in order to un-get tokens */
static int jp_token_stack_capacity = JP_TOKEN_STACK_CAPACITY;
static int jp_token_stack_size = 0;
static int jp_token_stack_ptr = 0;
static struct json_token jp_token_stack[JP_TOKEN_STACK_CAPACITY];

static int jp_stack_push(struct json_token* tok);
static struct json_token *jp_stack_pop();

/* functions to return token primitives */
static struct json_token *jp_empty_token();
static struct json_token *jp_eof_token();
static struct json_token *jp_open_brace_token();
static struct json_token *jp_close_brace_token();
static struct json_token *jp_open_bracket_token();
static struct json_token *jp_close_bracket_token();
static struct json_token *jp_string_token();
static struct json_token *jp_number_token();
static struct json_token *jp_colon_token();
static struct json_token *jp_comma_token();
static struct json_token *jp_undefined_token();
static struct json_token *jp_error_token(const char *msg);

#define JP_DEBUG_STACK_CAPACITY (20)

/* debug stack to debug code */
static int jp_debug_stack_capacity = JP_DEBUG_STACK_CAPACITY;
static int jp_debug_stack_size = 0;
static int jp_debug_stack_ptr = 0;
static const char *jp_debug_stack[JP_DEBUG_STACK_CAPACITY];
static int jp_push_error_debug(const char *msg);
static const char *jp_pop_error_debug(void);

typedef enum BUFFER_ERRORS {
        NO_BUFFER_ERROR = 0, NULL_BUFFER_ERROR,
        DATA_BUFFER_ERROR, RESIZE_BUFFER_ERROR,
        ENUM_BUFFER_COUNT
} BUFFER_ERRORS;

/* operations on the buffer_t structure */
int init_buffer_t(buffer_t *buffer);
int clear_buffer_t(buffer_t *buffer);
int append_buffer_t(buffer_t *buffer, char c);
int resize_buffer_t(buffer_t *buffer);
int write_buffer_t(buffer_t *buffer, const char *data);
int insert_buffer_t(buffer_t *buffer, const char *data, int offset);
int free_buffer_t(buffer_t *buffer);
static const char *get_error_buffer_t(int status);

/* initialise a buffer type, returns -1 on error,
   otherwise 0 on success. */
int init_buffer_t(buffer_t *buffer)
{
        if (buffer == NULL) {
                jp_push_error_debug(get_error_buffer_t(NULL_BUFFER_ERROR));
                return NULL_BUFFER_ERROR;
        }

        buffer->data = malloc(sizeof(*buffer->data) * (BUFFER_T_CAPACITY + 1));
        if (buffer->data == NULL) {
                free_buffer_t(buffer);
                jp_push_error_debug(get_error_buffer_t(DATA_BUFFER_ERROR));
                return DATA_BUFFER_ERROR;
        }

        buffer->size = 0;
        buffer->capacity = BUFFER_T_CAPACITY;
        return clear_buffer_t(buffer);
}

/* clear the contents of the data inside the buffer,
   returns -1 on error, 0 on success. */
int clear_buffer_t(buffer_t *buffer)
{
        if (buffer == NULL) {
                jp_push_error_debug(get_error_buffer_t(NULL_BUFFER_ERROR));
                return NULL_BUFFER_ERROR;
        }

        if (buffer->data == NULL) {
                jp_push_error_debug(get_error_buffer_t(DATA_BUFFER_ERROR));
                return DATA_BUFFER_ERROR;
        }

        memset(buffer->data, 0,
               (buffer->capacity + 1) * sizeof(*buffer->data));
        buffer->size = 0;
        return NO_BUFFER_ERROR;
}

/* appends character @c to the end of the @buffer,
   returns -1 on error, otherwise 0 on success */
int append_buffer_t(buffer_t *buffer, char c)
{
        if (buffer == NULL) {
                jp_push_error_debug(get_error_buffer_t(NULL_BUFFER_ERROR));
                return NULL_BUFFER_ERROR;
        }

        if (buffer->size >= buffer->capacity) {
                if (resize_buffer_t(buffer) != 0) {
                        jp_push_error_debug(get_error_buffer_t(RESIZE_BUFFER_ERROR));
                        return RESIZE_BUFFER_ERROR;
                }
        }

        buffer->data[buffer->size++] = c;
        buffer->data[buffer->size] = '\0';
        return NO_BUFFER_ERROR;
}

/* resizes the size of the buffer, to 2x the size,
   returns -1 on error, otherwise 0 on success */
int resize_buffer_t(buffer_t *buffer)
{
        char *new_data = realloc(buffer->data, (buffer->capacity*2) + 1);
        if (new_data == NULL) {
                jp_push_error_debug(get_error_buffer_t(RESIZE_BUFFER_ERROR));
                return RESIZE_BUFFER_ERROR;
        }

        buffer->data = new_data;
        buffer->capacity *= 2;
        return NO_BUFFER_ERROR;
}

/* writes the contents of data to the start of the buffer,
   equivalent to the call of: insert_buffer_t(buffer, data, 0),
   returns -1 on error, otherwise 0 on success */
int write_buffer_t(buffer_t *buffer, const char *data)
{
        return insert_buffer_t(buffer, data, 0);
}

/* inserts the contents of @data into the buffer, which is
   offset by @offset. Returns -1 on error, otherwise 0 on success */
int insert_buffer_t(buffer_t *buffer, const char *data, int offset)
{
        if (buffer == NULL) {
                jp_push_error_debug(get_error_buffer_t(NULL_BUFFER_ERROR));
                return NULL_BUFFER_ERROR;
        }

        if (data != NULL) {
                int data_len = strlen(data) + offset;
                if (data_len >= buffer->capacity) {
                        int status;
                        if ((status = resize_buffer_t(buffer)) != NO_BUFFER_ERROR)
                                return status;
                }

                strncpy(buffer->data+offset, data, buffer->capacity);
                buffer->size = data_len;
                buffer->data[data_len] = '\0';
        }

        return NO_BUFFER_ERROR;
}

/* frees the contents of the buffer, if @buffer is
   a malloc'd memory area, it must be free'd by the
   caller. returns -1 on error, otherwise 0 on success */
int free_buffer_t(buffer_t *buffer)
{
        if (buffer == NULL) {
                jp_push_error_debug(get_error_buffer_t(NULL_BUFFER_ERROR));
                return NULL_BUFFER_ERROR;
        }

        if (buffer->data != NULL) {
                clear_buffer_t(buffer);
                free(buffer->data);
                buffer->data = NULL;
        }

        buffer->size = 0;
        buffer->capacity = 0;
        return NO_BUFFER_ERROR;
}

/* returns a message describing the error code
   defined by @status */
static const char *get_error_buffer_t(int status)
{
        static const char *msgs[ENUM_BUFFER_COUNT] = {
                "No Error",
                "buffer was null!",
                "data was null!",
                "failed to resize buffer!",
        };

        return (status >= 0 && status < ENUM_BUFFER_COUNT
                ? msgs[status] : "Unknown Error");
}

/* push a json_token on top of the json parser stack,
   */
static int jp_stack_push(struct json_token* tok)
{
        if (jp_token_stack_size < jp_token_stack_capacity)
                jp_token_stack_size++;
        jp_token_stack[jp_token_stack_ptr].type = tok->type;
        if (write_buffer_t(&jp_token_stack[jp_token_stack_ptr].token, tok->token.data) != 0)
                return -1;
        jp_token_stack_ptr = (jp_token_stack_ptr + 1) % jp_token_stack_capacity;
        return 0;
}

static struct json_token *jp_stack_pop()
{
        if (jp_token_stack_size == 0) return NULL;
        jp_token_stack_ptr--;
        jp_token_stack_size--;
        if (jp_token_stack_ptr < 0)
                jp_token_stack_ptr += jp_token_stack_capacity;
        tok->type = jp_token_stack[jp_token_stack_ptr].type;
        write_buffer_t(&tok->token, jp_token_stack[jp_token_stack_ptr].token.data);
        return tok;
}

static struct json_token *jp_empty_token()
{
        if (tok == NULL) {
                tok = malloc(sizeof(*tok));
                if (tok == NULL)
                        return NULL;

                if (init_buffer_t(&tok->token) != 0) {
                        free(tok);
                        return NULL;
                }
        }

        tok->type = JSON_EMPTY;
        clear_buffer_t(&tok->token);
        return tok;
}

static struct json_token *jp_eof_token()
{
        tok = jp_empty_token();
        tok->type = JSON_EOF;
        write_buffer_t(&tok->token, "EOF");
        lookahead = fgetc(curr_fd);
        return tok;
}

static struct json_token *jp_open_brace_token()
{
        tok = jp_empty_token();
        tok->type = JSON_OPEN_BRACE;
        write_buffer_t(&tok->token, "{");
        lookahead = fgetc(curr_fd);
        return tok;
}

static struct json_token *jp_close_brace_token()
{
        tok = jp_empty_token();
        tok->type = JSON_CLOSE_BRACE;
        write_buffer_t(&tok->token, "}");
        lookahead = fgetc(curr_fd);
        return tok;
}

static struct json_token *jp_open_bracket_token()
{
        tok = jp_empty_token();
        tok->type = JSON_OPEN_BRACKET;
        write_buffer_t(&tok->token, "[");
        lookahead = fgetc(curr_fd);
        return tok;
}

static struct json_token *jp_close_bracket_token()
{
        tok = jp_empty_token();
        tok->type = JSON_CLOSE_BRACKET;
        write_buffer_t(&tok->token, "]");
        lookahead = fgetc(curr_fd);
        return tok;
}

static struct json_token *jp_string_token()
{
        tok = jp_empty_token();
        tok->type = JSON_STRING;
        lookahead = fgetc(curr_fd);
        while (lookahead != '"' && lookahead != EOF) {
                append_buffer_t(&tok->token, lookahead);
                lookahead = fgetc(curr_fd);
        }

        if (lookahead == EOF) {
                tok = jp_error_token("Unterminated string");
        } else {
                lookahead = fgetc(curr_fd);
        }
        return tok;
}

static struct json_token *jp_number_token()
{
        tok = jp_empty_token();
        tok->type = JSON_NUMBER;
        while ((lookahead >= '0' && lookahead <= '9') && lookahead != EOF) {
                append_buffer_t(&tok->token, lookahead);
                lookahead = fgetc(curr_fd);
        }

        if (lookahead == EOF) {
                tok = jp_error_token("Unterminated number");
                return tok;
        }

        if (lookahead == '.') {
                append_buffer_t(&tok->token, lookahead);
                lookahead = fgetc(curr_fd);
                while ((lookahead >= '0' && lookahead <= '9') && lookahead != EOF) {
                        append_buffer_t(&tok->token, lookahead);
                        lookahead = fgetc(curr_fd);
                }

                if (lookahead == EOF) {
                        tok = jp_error_token("Unterminated number");
                        return tok;
                }
        }
        return tok;
}

static struct json_token *jp_colon_token()
{
        tok = jp_empty_token();
        tok->type = JSON_COLON;
        write_buffer_t(&tok->token, ":");
        lookahead = fgetc(curr_fd);
        return tok;
}

static struct json_token *jp_comma_token()
{
        tok = jp_empty_token();
        tok->type = JSON_COMMA;
        write_buffer_t(&tok->token, ",");
        lookahead = fgetc(curr_fd);
        return tok;
}

static struct json_token *jp_undefined_token()
{
        tok = jp_empty_token();
        tok->type = JSON_UNDEFINED;
        write_buffer_t(&tok->token, "UNDEFINED");
        lookahead = fgetc(curr_fd);
        return tok;
}

static struct json_token *jp_error_token(const char *msg)
{
        tok = jp_empty_token();
        tok->type = JSON_EOF;
        if (msg != NULL)
                write_buffer_t(&tok->token, msg);
        else
                write_buffer_t(&tok->token, "ERROR");
        return tok;
}

int jp_set_fd(FILE *fd)
{
        for (int i = 0; i < jp_token_stack_capacity; i++) {
                init_buffer_t(&jp_token_stack[i].token);
                jp_token_stack[i].type = JSON_EMPTY;
        }

        curr_fd = fd;
        if (curr_fd == NULL) return -1;
        lookahead = fgetc(curr_fd);
        return 0;
}

struct json_token *jp_peek_token()
{
        jp_stack_push(jp_get_token());
        return tok;
}

struct json_token *jp_get_token()
{
        if (jp_token_stack_size > 0)
                return jp_stack_pop();
        if (curr_fd == NULL) return jp_empty_token();

        // skip whitespace.
        while (lookahead == ' ' || lookahead == '\t' || lookahead == '\n' || lookahead == '\r')
                lookahead = fgetc(curr_fd);

        if (lookahead == EOF) {
                return jp_eof_token();
        } else if (lookahead == '{') {
                return jp_open_brace_token();
        } else if (lookahead == '}') {
                return jp_close_brace_token();
        } else if (lookahead == '[') {
                return jp_open_bracket_token();
        } else if (lookahead == ']') {
                return jp_close_bracket_token();
        } else if (lookahead == ',') {
                return jp_comma_token();
        } else if (lookahead == ':') {
                return jp_colon_token();
        } else if (lookahead == '"') {
                return jp_string_token();
        } else if (lookahead >= '0' && lookahead <= '9') {
                return jp_number_token();
        }

        return jp_undefined_token();
}

struct json_token *jp_unget_token(struct json_token *tok)
{
        jp_stack_push(tok);
        return tok;
}

/* rewinds fd back to the beginning of the file, equivalent of a
  call to rewind(fd) or fseek(fd, 0L, SEEK_SET) */
int jp_rewind()
{
        if (curr_fd == NULL) return -1;
        rewind(curr_fd);
        return 0;
}
