#ifndef _LAFFTALE_HTTP_VERSION_H_
#define _LAFFTALE_HTTP_VERSION_H_

#include "http_common.h"

#define LHTTP_V_1_1_STR "1.1"

/**
 * \brief Version enumeration
 */
typedef enum _http_version_e_ {
    HTTP_VERSION_1_1
} http_version;

/**
 * \brief Version structure with str and enumeration
 */
typedef struct _http_version_ {
    const char   *str;  /**Version string */
    http_version  num;  /**Version enumeration */
} http_version_t;

/**
 * \brief Parses string to initialize http_version_t
 *
 * \param *out http_version to initialize
 * \param *str str for initializing
 *
 * \return LHTTP_SUCCESS | LHTTP_UNKOWN_VERSION | LHTTP_STATIC_ERR | LHTTP_STR_MALFORMED |
 * LHTTP_STR_MALFORMED
 */
int http_version_str_to_v(http_version_t *out, const char *str);

#endif
