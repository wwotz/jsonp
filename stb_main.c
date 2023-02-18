#define JSONP_IMPLEMENTATION
#include "stb_jsonp.h"

int main(int argc, char **argv)
{
        if (jsonp_init(jsonp_create_json_info(JSONP_FILE,
                                              "sample.json")) != JSONP_NO_ERROR) {
                fprintf(stderr, "Failed!\n");
                exit(EXIT_FAILURE);
        }
        return 0;
}
