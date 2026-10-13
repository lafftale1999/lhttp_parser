#include "http_method.h"

static const http_method_t http_methods_map[] = {
    {.num = HTTP_METHOD_GET,    .str = "GET"},
    {.num = HTTP_METHOD_PUT,    .str = "PUT"},
    {.num = HTTP_METHOD_POST,   .str = "POST"},
    {.num = HTTP_METHOD_DELETE, .str = "DELETE"}
};

static const size_t http_methods_map_size = sizeof(http_methods_map) / sizeof(http_methods_map[0]);

static int http_method_check_boundaries(const char *str) {
    size_t size = 0;

    while (size++ < HTTP_METHOD_MAX_STR_SIZE) {
        if (str[size - 1] == '\0') {
            return 0;
        }
    }

    return -1;
}

int http_method_str_to_m(http_method_t *out, const char *str) {
    int res = LHTTP_UNKOWN_METHOD;

    if (str == NULL) {
        fprintf(stderr, "http_method_str_to_m failed, str is NULL\n");
        return LHTTP_STATIC_ERR;
    }

    if (http_method_check_boundaries(str) != 0) {
        fprintf(stderr, "http_method_str_to_m failed, boundary check failed\n");
        return LHTTP_STR_MALFORMED;
    }

    for (size_t idx = 0; idx < http_methods_map_size; idx++) {
        const http_method_t *tmp = &http_methods_map[idx];

        if (!strncmp(str, tmp->str, strlen(tmp->str))) {
            memcpy(out, tmp, sizeof(*out));
            return LHTTP_SUCCESS;
        }
    }

    fprintf(stderr, "http_method_str_to_m failed, unkown method\n");
    return LHTTP_UNKOWN_METHOD;

    return res;
}