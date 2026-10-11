#include "http_response.h"
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>

#define SPACE_SIZE 1

static size_t http_response_get_size(http_response_t *in) {
    static const size_t crlf = sizeof("\r\n") - 1;
    static const size_t h_sep = sizeof(": ") - 1;

    size_t size = 0;

    // Version
    size += strlen("HTTP/") + strlen(http_version_v_to_str(in->version)) + SPACE_SIZE;
    size += LHTTP_RESPONSE_CODE_NUM_SIZE + SPACE_SIZE + strlen(in->response_code.str);
    size += crlf;
    
    // Headers
    for (size_t idx = 0; idx < in->headers.len; idx++) {
        http_header_t *h = &in->headers.headers[idx];
        size += h->key_len + h_sep + h->val_len + crlf;
    }

    // Stop
    size += crlf;

    // Body
    size += in->body.size;

    return size;
}

__attribute__((format(printf, 4, 5)))
static int http_response_write_line(char **pos,
                                    size_t remaining,
                                    size_t *written,
                                    const char *format, ...) {
    va_list args;
    int res;

    /* remaining excludes the '\0' slot reserved at the end of the buffer */
    va_start(args, format);
    res = vsnprintf(*pos, remaining + 1, format, args);
    va_end(args);

    if (res < 0) {
        fprintf(stderr, "http_response_write_line failed: %s\n", strerror(errno));
        *written = 0;
        return LHTTP_UNKOWN_ERROR;
    }

    if ((size_t) res > remaining) {
        fprintf(stderr, "http_response_write_line failed, allocated memory is too small\n");
        *written = (size_t) res;   /* bytes that were needed, excluding '\0' */
        return LHTTP_BUFFER_OVERFLOW;
    }

    *written = (size_t) res;
    *pos += res;

    return LHTTP_SUCCESS;
}

int http_response_to_data(http_response_t *in, const char **data, size_t *size_out) {
    char *tmp;
    char *pos;
    int res = 0;
    int idx = 0;

    size_t data_size = 0;
    size_t remaining = 0;
    size_t written = 0;

    if (in == NULL) {
        fprintf(stderr, "http_response_to_data failed, in is NULL\n");
        return LHTTP_STATIC_ERR;
    }

    if (in->headers.len > 0 && in->headers.headers == NULL) {
        fprintf(stderr, "http_response_to_data failed, headers are malformed\n");
        return LHTTP_STR_MALFORMED;
    }

    if (in->body.size > 0 && in->body.data == NULL) {
        fprintf(stderr, "http_response_to_data failed, body is malformed\n");
        return LHTTP_STR_MALFORMED;
    }
    
    data_size = http_response_get_size(in);

    tmp = calloc(data_size + 1, sizeof(char));
    if (tmp == NULL) {
        fprintf(stderr, "http_response_to_data failed, unable to allocate memory\n");
        return LHTTP_MEMORY_ERR;
    }

    pos = tmp;
    remaining = data_size;

    // FIRST LINE
    res = http_response_write_line(&pos, remaining, &written,
                                   "HTTP/%s %d %s\r\n",
                                   http_version_v_to_str(in->version),
                                   in->response_code.num,
                                   in->response_code.str);
    if (res != LHTTP_SUCCESS) {
        fprintf(stderr, "http_response_to_data failed, failed to write first line\n");
        goto cleanup;
    }
    remaining -= written;

    // HEADERS
    for (size_t i = 0; i < in->headers.len; i++) {
        http_header_t *h = &in->headers.headers[i];
        res = http_response_write_line(&pos, remaining, &written, "%.*s: %.*s\r\n",
                                       (int) h->key_len, h->key,
                                       (int) h->val_len, h->val);

        if (res != LHTTP_SUCCESS) {
            fprintf(stderr, "http_response_to_data failed, failed to add header\n");
            goto cleanup;
        }

        remaining -= written;
    }

    if ((res = http_response_write_line(&pos, remaining, &written, "\r\n")) != LHTTP_SUCCESS) {
        fprintf(stderr, "http_response_to_data failed, failed to add stop\n");
        goto cleanup;
    }

    remaining -= written;

    // BODY
    if (in->body.size > 0) {
        if (in->body.size > remaining) {
            fprintf(stderr, "http_response_to_data failed, failed to add body\n");
            res = LHTTP_BUFFER_OVERFLOW;
            goto cleanup;
        }

        memcpy(pos, in->body.data, in->body.size);
        pos += in->body.size;
        remaining -= in->body.size;
    }

    if (remaining != 0) {
        fprintf(stderr, "http_response_to_data failed, remaining is not 0: state=%lu\n", remaining);
        res = LHTTP_STR_MALFORMED;
        goto cleanup;
    }

    *data = tmp;
    if (size_out != NULL) {
        *size_out = data_size;
    }

    return LHTTP_SUCCESS;

cleanup:
    if (tmp != NULL) {
        free(tmp);
    }

    return res;
}

int http_response_free(http_response_t *resp) {
    int res;
    
    if (resp == NULL) {
        fprintf(stderr, "http_response_free failed, resp is NULL\n");
        return LHTTP_STATIC_ERR;
    }

    if ((res = http_body_free(&resp->body)) != LHTTP_SUCCESS) {
        fprintf(stderr, "http_response_free failed to free body: %d\n", res);
    }

    if ((res = http_headers_arr_free(&resp->headers)) != LHTTP_SUCCESS) {
        fprintf(stderr, "http_response_free failed to free headers: %d\n", res);
    }

    return res;
}