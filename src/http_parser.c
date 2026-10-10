#include "http_parser.h"

/**
 * \brief Find the end of headers, and beginning of body.
 */
static int http_parser_find_body(const char *pos, const char *end, const char **out) {
    size_t count = 0;

    while (pos < end && count++ < LHTTP_HEADER_MAX_SIZE) {
        if (*pos == '\r') {
            if ((pos + 4) > end) {
                fprintf(stderr, "http_parser_find_body failed, request incomplete\n");
                return LHTTP_REQUEST_INCOMPLETE;
            }

            if (*(pos + 1) == '\n' &&
                *(pos + 2) == '\r' &&
                *(pos + 3) == '\n') {
                    *out = pos + 4;
                    return LHTTP_SUCCESS;
                }
        }

        pos++;
    }

    if (count >= LHTTP_HEADER_MAX_SIZE) return LHTTP_HEADER_TOO_LARGE;

    return LHTTP_REQUEST_INCOMPLETE;
}

static int http_parser_find_body_end(const char *end, const char **out) {
    if ((*(end - 1) == '\r') && (*(end) == '\n')) {
        *out = end - 2;
        return LHTTP_SUCCESS;
    }

    return LHTTP_REQUEST_INCOMPLETE;
}

static int http_parser_next_line(const char *pos, const char *end, const char **out, const char **next_start) {
    while (pos < end) {
        if (*pos == '\r') {
            if (*(pos + 1) != '\n') {
                fprintf(stderr, "http_parser_next_line failed, data is malformed\n");
                return LHTTP_STR_MALFORMED;
            }

            *out = pos - 1;
            
            if ((*(pos + 1) == '\n') &&
                (*(pos + 2) == '\r') &&
                (*(pos + 3) == '\n')) {
                *next_start = NULL;
                return LHTTP_SUCCESS;
            }

            if ((pos + 2) < end)
                *next_start = (pos + 2);
            else
                *next_start = NULL;

            return LHTTP_SUCCESS;
        }

        pos++;
    }

    return LHTTP_REQUEST_INCOMPLETE;
}

static int http_parser_request_first_line(http_request_t *out, const char *line, size_t line_size) {
    const char delim = ' ';
    const char *pos = line;
    const char *end = pos + line_size;

    int res;
    
    char buf[512];
    size_t tmp_size;
    
    const char *pos_arr[3];
    size_t idx = 0;

    pos_arr[idx++] = pos;

    while (pos < end && idx < 3) {
        if (*pos == delim) {
            pos_arr[idx++] = (pos + 1);
        }
        pos++;
    }

    if (idx < 3) {
        fprintf(stderr, "http_parser_request_firs_line failed, line is malformed\n");
        return LHTTP_STR_MALFORMED;
    }

    // ADD METHOD
    tmp_size = pos_arr[1] - pos_arr[0];
    memcpy(buf, pos_arr[0], tmp_size);
    buf[tmp_size - 1] = '\0';
    res = http_method_str_to_m(buf);
    if (res < 0) {
        fprintf(stderr, "http_parser_request_first_line failed, method is malformed\n");
        return res;
    }

    out->method = res;

    // ADD PATH
    tmp_size = pos_arr[2] - pos_arr[1];
    memcpy(buf, pos_arr[1], tmp_size);
    buf[tmp_size - 1] = '\0';
    res = http_path_str_to_p(&out->path, buf, tmp_size);
    if (res < 0) {
        fprintf(stderr, "http_parser_request_first_line failed, path is malformed\n");
        return res;
    }

    // ADD VERSION
    tmp_size = end - pos_arr[2] + 1;
    memcpy(buf, pos_arr[2], tmp_size);
    buf[tmp_size] = '\0';
    res = http_version_str_to_v(buf);
    if (res < 0) {
        fprintf(stderr, "http_parser_request_first_line failed, version is malformed\n");
        (void)http_path_free(&out->path);
        return res;
    }

    out->version = res;

    return LHTTP_SUCCESS;
}

int http_parser_data_to_request(http_request_t *out, const char *data, size_t data_len) {
    const char *data_start, *data_end;
    const char *line_start, *new_line_start, *line_end;
    const char *body_start, *body_end;
    const char *content_length_str;
    int content_length_i;
    int res, idx;
    
    if (out == NULL) {
        fprintf(stderr, "http_parser_data_to_request failed, out is NULL\n");
        return LHTTP_STATIC_ERR;
    }

    if (data == NULL) {
        fprintf(stderr, "http_parser_data_to_request failed, data is NULL\n");
        return LHTTP_STATIC_ERR;
    }

    data_start = data;
    data_end = data + data_len;

    res = http_parser_find_body(data_start, data_end, &body_start);
    if (res < 0) {
        fprintf(stderr, "http_parser_data_to_request failed, unable to find body\n");
        return res;
    }

    res = http_parser_find_body_end(data_end, &body_end);
    if (res < 0) {
        fprintf(stderr, "http_parser_data_to_request failed, body is inclomplete\n");
        return res;
    }

    // FIRST LINE
    line_start = data_start;
    res = http_parser_next_line(line_start, body_start, &line_end, &new_line_start);
    if (res < 0) {
        fprintf(stderr, "http_parser_data_to_request failed, unable to parse first line\n");
        goto cleanup;
    }

    res = http_parser_request_first_line(out, line_start, (size_t)(line_end - line_start));
    if (res < 0) {
        fprintf(stderr, "http_parser_data_to_request failed, unable to initialize first line\n");
        goto cleanup;
    }

    // HEADERS
    while (new_line_start != NULL && new_line_start < body_start) {
        http_header_t tmp = {0};
        line_start = new_line_start;
        res = http_parser_next_line(line_start, body_start, &line_end, &new_line_start);

        res = http_header_parse_line_to_header(&tmp, line_start, (size_t)(line_end - line_start + 1));
        if (res < 0) {
            fprintf(stderr, "http_parser_data_to_request failed, unable to parse line to header\n");
            goto cleanup;
        }

        res = http_headers_arr_append(&out->headers, &tmp);
        if (res < 0) {
            fprintf(stderr, "http_parser_data_to_request failed, unable to add header\n");
            goto cleanup;
        }
    }

    idx = http_headers_arr_find(&out->headers, "Content-Length");

    if (idx >= 0) {
        content_length_str = out->headers.headers[idx].val;
        content_length_i = strtol(content_length_str, NULL, 10);

        if (content_length_i > 0) {
            res = http_body_initialize(&out->body, body_start, (size_t)(body_end - body_start));
            if (res < 0) {
                fprintf(stderr, "http_parser_data_to_request failed, unable to initialize body\n");
                goto cleanup;
            }

            if ((size_t)(content_length_i) != out->body.size) {
                fprintf(stderr, "http_parser_data_to_request failed, body is malformed: %lu | %lu\n", (size_t)(content_length_i), out->body.size);
                goto cleanup;
            }
        }
    }

    return LHTTP_SUCCESS;

cleanup:
    if (out != NULL) {
        http_request_free(out);
    }

    return res;
}
