#ifndef _LAFFTALE_HTTP_REQUEST_H_
#define _LAFFTALE_HTTP_REQUEST_H_

#include "http_base_content.h"

typedef struct _http_request_ {
    http_method_t       method;
    http_path_t         path;
    http_version_t      version;
    http_headers_arr_t  headers;
    http_body_t         body;
} http_request_t;

int http_request_free(http_request_t *req);
int http_request_from_data(http_request_t *out, const char *data, size_t data_len);

#endif