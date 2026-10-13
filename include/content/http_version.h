#ifndef _LAFFTALE_HTTP_VERSION_H_
#define _LAFFTALE_HTTP_VERSION_H_

#include "http_common.h"

typedef enum _http_version_e_ {
    HTTP_VERSION_1_1
} http_version;

typedef struct _http_version_ {
    const char   *str;
    http_version  num;
} http_version_t;

int http_version_str_to_v(http_version_t *out, const char *str);

#endif
