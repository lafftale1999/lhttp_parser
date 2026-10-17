#ifndef _LAFFTALE_HTTP_METHOD_H_
#define _LAFFTALE_HTTP_METHOD_H_

#include "http_common.h"

#define HTTP_METHOD_MAX_STR_SIZE    sizeof("DELETE")

/**
 * \brief Method enumeration
 */
typedef enum _http_method_ {
    HTTP_METHOD_GET,
    HTTP_METHOD_POST,
    HTTP_METHOD_PUT,
    HTTP_METHOD_DELETE
} http_method;

/**
 * \brief Method structure to get both str and enumerated value.
 */
typedef struct _http_method_t_ {
    const char  *str;   /** Version str */
    http_method  num;   /** Version enum */
} http_method_t;

/**
 * \brief Initializes the http_method_t
 *
 * \param *out method to initialize
 * \param *str version string
 *
 * \return LHTTP_SUCCESS | LHTTP_STATIC_ERR | LHTTP_STR_MALFORMED | LHTTP_UNKOWN_METHOD
 */
int http_method_str_to_m(http_method_t *out, const char *str);

#endif