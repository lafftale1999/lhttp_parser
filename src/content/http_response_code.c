#include "http_response_code.h"

int http_response_code_parse_num(http_response_code_t *out, int num) {
    if (out == NULL) {
        fprintf(stderr, "http_response_code_parse_str failed, out is NULL\n");
        return LHTTP_STATIC_ERR;
    }

    for (size_t idx = 0; idx < response_code_map_len; idx++) {
        const http_response_code_t *tmp = &response_code_map[idx];
        if (num == tmp->num) {
            out->num = tmp->num;
            out->str = tmp->str;

            return LHTTP_SUCCESS;
        }
    }

    return LHTTP_NOT_FOUND;
}