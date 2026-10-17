#include "http_request.h"

// headers end offset characters
#define HEADERS_END_OFFSET (sizeof("\r\n\r\n") - 1)

/**
 * \brief Find the end of headers, and beginning of body.
 * 
 * \param *pos start position
 * \param *end end position
 * \param **out set pointer to last element before end of headers
 * 
 * \return LHTTP_SUCCESS | LHTTP_HEADER_TOO_LARGE | LHTTP_REQUEST_INCOMPLETE
 */
static int http_request_find_headers_end(const char *pos, const char *end, const char **out) {
    size_t count = 0;

    while ((pos + 3) < end && count++ < LHTTP_HEADER_MAX_SIZE) {
        if (*pos       == '\r' &&
            *(pos + 1) == '\n' &&
            *(pos + 2) == '\r' &&
            *(pos + 3) == '\n') {

                *out = pos - 1;
                return LHTTP_SUCCESS;
            }

        pos++;
    }

    if (count >= LHTTP_HEADER_MAX_SIZE) return LHTTP_HEADER_TOO_LARGE;

    return LHTTP_REQUEST_INCOMPLETE;
}

/**
 * \brief Find the next line in the request. The line will exist between `pos` and `line_end_out`.
 * set position of next starting point in `next_start`.
 * 
 * If the header section has been fully received, `next_start` will be NULL.
 * 
 * \param *pos Initial position to start searching from.
 * \param *end End of range to search within
 * \param **line_end_out pointer to where the line ends
 * \param **next_start poitner to where next line starts.
 * 
 * \return LHTTP_SUCCESS | LHTTP_STR_MALFORMED | LHTTP_REQUEST_INCOMPLETE
 */
static int http_request_next_line(const char *pos, const char *end, const char **line_end_out, const char **next_start) {
    while (pos < end) {
        if (*pos == '\r') {
            // Check format and restricted range
            if ((pos + 3) > end ||
                *(pos + 1) != '\n') {
                    fprintf(stderr, "http_request_next_line failed, data is malformed\n");
                    return LHTTP_STR_MALFORMED;

                }
            
            *line_end_out = pos - 1;
            
            // Check if we've reached header ends
            if ((*(pos + 2) == '\r') &&
                (*(pos + 3) == '\n')) {
                *next_start = NULL;
                return LHTTP_SUCCESS;
            }

            // If not set next position
            *next_start = (pos + 2);
            return LHTTP_SUCCESS;
        }

        pos++;
    }

    return LHTTP_REQUEST_INCOMPLETE;
}

/**
 * \brief Parses the first request line to set METHOD, PATH and VERSION
 * 
 * \param *out Request to initialize.
 * \param *line Line to parse
 * \param line_size Size of Line
 * 
 * \return LHTTP_SUCCESS | LHTTP_STR_MALFORMED | LHTTP_STATIC_ERR | LHTTP_UNKOWN_METHOD
 * LHTTP_MEMORY_ERR | LHTTP_UNKOWN_VERSION
 */
static int http_request_request_first_line(http_request_t *out, const char *line, size_t line_size) {
    const char delim = ' ';
    const char *pos = line;
    const char *end = pos + line_size;

    int res;

    char buf[512];
    size_t tmp_size;

    const char *pos_arr[3];
    size_t idx = 0;

    // Split line by spaces
    pos_arr[idx++] = pos;
    while (pos < end && idx < 3) {
        if (*pos == delim) {
            pos_arr[idx++] = (pos + 1);
        }
        pos++;
    }

    if (idx < 3) {
        fprintf(stderr, "http_request_request_firs_line failed, line is malformed\n");
        return LHTTP_STR_MALFORMED;
    }

    // ADD METHOD
    tmp_size = pos_arr[1] - pos_arr[0];
    memcpy(buf, pos_arr[0], tmp_size);
    buf[tmp_size - 1] = '\0';
    res = http_method_str_to_m(&out->method, buf);
    if (res < 0) {
        fprintf(stderr, "http_request_request_first_line failed, method is malformed\n");
        return res;
    }

    // ADD PATH
    tmp_size = pos_arr[2] - pos_arr[1];
    memcpy(buf, pos_arr[1], tmp_size);
    buf[tmp_size - 1] = '\0';
    res = http_path_str_to_p(&out->path, buf, tmp_size);
    if (res < 0) {
        fprintf(stderr, "http_request_request_first_line failed, path is malformed\n");
        return res;
    }

    // ADD VERSION
    tmp_size = end - pos_arr[2] + 1;
    memcpy(buf, pos_arr[2], tmp_size);
    buf[tmp_size] = '\0';
    res = http_version_str_to_v(&out->version, buf);
    if (res < 0) {
        fprintf(stderr, "http_request_request_first_line failed, version is malformed\n");
        (void)http_path_free(&out->path);
        return res;
    }

    return LHTTP_SUCCESS;
}

int http_request_from_data(http_request_t *out, const char *data, size_t data_len) {
    const char *data_start, *data_end;
    const char *line_start, *new_line_start, *line_end;
    const char *headers_end;
    const char *body_start;

    long res, body_size;
    int idx;

    if (out == NULL) {
        fprintf(stderr, "http_request_from_data failed, out is NULL\n");
        return LHTTP_STATIC_ERR;
    }

    if (data == NULL) {
        fprintf(stderr, "http_request_from_data failed, data is NULL\n");
        return LHTTP_STATIC_ERR;
    }

    data_start = data;
    data_end = data_start + data_len;

    // bytes_parsed == 0 -> headers not parsed yet.
    // Otherwise bytes_parsed holds the size of the header section (incl. "\r\n\r\n")
    if (out->bytes_parsed == 0) {
        if (out->headers.headers != NULL) {
            fprintf(stderr, "http_request_from_data failed, headers are not NULL\n");
            return LHTTP_REQUEST_MALFORMED;
        }

        // Ensure headers has been received
        res = http_request_find_headers_end(data_start, data_end, &headers_end);
        if (res < 0) {
            fprintf(stderr, "http_request_from_data failed, find_headers res: %ld\n", res);
            return res;
        }

        // Ensure headers aren't larger than allowed data size
        if (((size_t)(headers_end - data_start)) > LHTTP_REQUEST_MAX_SIZE) {
            fprintf(stderr, "http_request_from_data failed, headers section larger than allowed size\n");
            return LHTTP_REQUEST_TOO_LARGE;
        }

        // Parse first line
        res = http_request_next_line(data_start, data_end, &line_end, &new_line_start);
        if (res < 0) {
            fprintf(stderr, "http_request_from_data failed, next_line res: %ld\n", res);
            return res;
        }

        res = http_request_request_first_line(out, data_start, (size_t) (line_end - data_start));
        if (res < 0) {
            fprintf(stderr, "http_request_from_data failed, first_line res: %ld\n", res);
            return res;
        }

        // Add headers
        while (new_line_start != NULL && new_line_start <= headers_end) {
            line_start = new_line_start;
            res = http_request_next_line(line_start, data_end, &line_end, &new_line_start);
            if (res < 0) {
                fprintf(stderr, "http_request_from_data failed, unable to get next line: %ld\n", res);
                http_request_free(out);
                return res;
            }

            // Check that header isn't larger than allowed size
            if (((size_t)(line_end - line_start)) > LHTTP_HEADER_MAX_SIZE) {
                fprintf(stderr, "http_request_from_data failed, header line too large\n");
                return LHTTP_HEADER_TOO_LARGE;
            }

            // line_end points at the last character of the line (inclusive)
            res = http_headers_arr_append_str(&out->headers, line_start, (size_t) (line_end - line_start + 1));
            if (res < 0) {
                fprintf(stderr, "http_request_from_data failed, unable to add header\n");
                http_request_free(out);
                return res;
            }
        }

        // headers_end points at the last character before "\r\n\r\n"
        out->bytes_parsed = (size_t) (headers_end - data_start + 1) + HEADERS_END_OFFSET;
    }

    // No Content-Length -> no body, request is complete
    idx = http_headers_arr_find(&out->headers, "Content-Length");
    if (idx < 0) {
        return LHTTP_SUCCESS;
    }

    body_size = strtol(out->headers.headers[idx].val, NULL, 10);
    if (body_size < 0) {
        fprintf(stderr, "http_request_from_data failed, content-length malformed\n");
        return LHTTP_REQUEST_MALFORMED;
    }

    if ((out->bytes_parsed + body_size) > LHTTP_REQUEST_MAX_SIZE) {
        fprintf(stderr, "http_request_from_data failed, request too large\n");
        return LHTTP_REQUEST_TOO_LARGE;
    }

    // Body begins right after the header section
    body_start = data_start + out->bytes_parsed;

    // Body not fully received yet, caller should call again with more data
    if ((size_t) (data_end - body_start) < (size_t) body_size) {
        return LHTTP_REQUEST_INCOMPLETE;
    }

    res = http_body_initialize(&out->body, body_start, (size_t) body_size);
    if (res < 0) {
        fprintf(stderr, "http_request_from_data failed, unable to initialize body\n");
        return res;
    }

    out->bytes_parsed += out->body.size;

    return LHTTP_SUCCESS;
}

int http_request_free(http_request_t *req) {
    int res = LHTTP_SUCCESS;

    if (req == NULL) {
        fprintf(stderr, "http_request_free failed, req is NULL\n");
        return LHTTP_STATIC_ERR;
    }

    if (http_body_free(&req->body) < 0) {
        fprintf(stderr, "http_request_free unable to free body\n");
        res = -1;
    }

    if (http_headers_arr_free(&req->headers) < 0) {
        fprintf(stderr, "http_request_free unable to free headers\n");
        res = -1;
    }

    if (http_path_free(&req->path) < 0) {
        fprintf(stderr, "http_request_free unable to free path\n");
        res = -1;
    }

    if (res < 0) {
        fprintf(stderr, "http_request_free one or more free failed\n");
    }

    return res;
}