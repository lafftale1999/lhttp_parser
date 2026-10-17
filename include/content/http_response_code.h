#ifndef _LAFFTALE_HTTP_RESPONSE_CODE_H_
#define _LAFFTALE_HTTP_RESPONSE_CODE_H_

#include "http_common.h"

#define LHTTP_RESPONSE_CODE_NUM_SIZE 3

/**
 * \brief Current internally supported response codes.
 */
typedef enum _supported_codes_ {
    RESPONSE_CODE_OK,
    RESPONSE_CODE_NOT_FOUND,
    RESPONSE_CODE_CONTENT_TOO_LARGE,
    RESPONSE_CODE_INTERNAL_SERVER_ERROR
} supported_response_codes_t;

/**
 * \brief response code structure
 */
typedef struct _http_response_code_ {
    const char *str;    /** pointer to static string */
    int         num;    /** response code number */
} http_response_code_t;

static const http_response_code_t response_code_map[] = {
    {.num = 200, .str = "OK"},
    {.num = 404, .str = "Not Found"},
    {.num = 413, .str = "Content Too Large"},
    {.num = 500, .str = "Internal Server Error"}
};

static const size_t response_code_map_len = sizeof(response_code_map) / sizeof(response_code_map[0]);

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
