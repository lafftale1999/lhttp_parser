#ifndef _LAFFTALE_HTTP_METHOD_H_
#define _LAFFTALE_HTTP_METHOD_H_

#include "http_common.h"

#define HTTP_METHOD_MAX_STR_SIZE    sizeof("DELETE")

typedef enum _http_method_ {
    HTTP_METHOD_GET,
    HTTP_METHOD_POST,
    HTTP_METHOD_PUT,
    HTTP_METHOD_DELETE
} http_method;

typedef struct _http_method_t_ {
    const char  *str;
    http_method  num;
} http_method_t;

int http_method_str_to_m(http_method_t *out, const char *str);

#endif