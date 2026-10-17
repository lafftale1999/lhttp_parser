#ifndef _LAFFTALE_HTTP_RESPONSE_CODE_H_
#define _LAFFTALE_HTTP_RESPONSE_CODE_H_

#include "http_common.h"

#define LHTTP_RESPONSE_CODE_NUM_SIZE 3

/**
 * \brief response code structure
 */
typedef struct _http_response_code_ {
    const char *str;    /** pointer to static string */
    int         num;    /** response code number */
} http_response_code_t;

/**
 * \brief Initialize a http_response_code with a number
 * 
 * \param *out response code to initialize
 * \param num response code number
 * 
 * \return LHTTP_SUCCESS | LHTTP_STATIC_ERR | LHTTP_NOT_FOUND
 */
int http_response_code_parse_num(http_response_code_t *out, int num);

#endif
