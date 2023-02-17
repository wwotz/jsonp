#include "jsonp.h"
#include <stdio.h>
#include <stdlib.h>

static int prettify(const char *file_path, const char *output_file)
{
        if (jp_open_file(file_path) != 0) {
                fprintf(stderr, "Invalid file name!\n");
                return -1;
        }

        struct json_token *tok;
        int tab_count = 0, ttab_count = tab_count;
        while ((tok = jp_get_token())->type != JSON_EOF) {
                switch (tok->type) {
                case JSON_OPEN_BRACE:
                        tab_count++;
                case JSON_COMMA:
                        printf("%s\n", tok->token.data);
                        ttab_count = tab_count;
                        while (ttab_count--) {
                                printf("\t");
                        }
                        break;
                case JSON_CLOSE_BRACE:
                        tab_count--;
                        ttab_count = tab_count;
                        printf("\n");
                        while (ttab_count--) {
                                printf("\t");
                        }
                        printf("%s\n", tok->token.data);
                        int ttab_count = tab_count;
                        while (ttab_count--) {
                                printf("\t");
                        }
                        break;
                case JSON_COLON:
                        printf(" : ");
                        break;
                case JSON_STRING:
                        printf("\"%s\"", tok->token.data);
                        break;
                default:
                        printf("%s", tok->token.data);
                        break;
                }
        }
        return 0;
}

int main(int argc, char **argv)
{
        if (prettify("sample.json", NULL) != 0) {
                return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
}
