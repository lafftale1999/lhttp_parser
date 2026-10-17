#ifndef _LAFFTALE_HTTP_REQUEST_H_
#define _LAFFTALE_HTTP_REQUEST_H_

#include "http_base_content.h"

/**
 * \brief Request structure, initialize by using `http_request_from_data()`
 */
typedef struct _http_request_ {
    http_method_t       method;         /** Method string and enumeration */
    http_path_t         path;           /** Path string */
    http_version_t      version;        /** Version string and enumeration */
    http_headers_arr_t  headers;        /** Array of headers with key and value */
    http_body_t         body;           /** Body string */
    size_t              bytes_parsed;   /** Bytes parsed */
} http_request_t;

/**
 * \brief Attempts to parse data and initialize the http_request_t *out if succesful.
 * 
 * \note This will allocate dynamic memory, must use `http_request_free()` if succesful.
 * 
 * \param *out http_request_t to initialize
 * \param *data array of bytes to parse
 * \param data_len length of data
 * 
 * \return LHTTP_SUCCESS | LHTTP_STATIC_ERR | LHTTP_REQUEST_MALFORMED | LHTTP_HEADER_TOO_LARGE |
 * LHTTP_REQUEST_INCOMPLETE | LHTTP_STR_MALFORMED | LHTTP_UNKOWN_METHOD | LHTTP_UNKOWN_VERSION |
 * LHTTP_MEMORY_ERR | LHTTP_REQUEST_TOO_LARGE
 */
int http_request_from_data(http_request_t *out, const char *data, size_t data_len);

/**
 * \brief Free's up all memory allocated for the request.
 * 
 * \param *req request to free up
 * 
 * \return LHTTP_SUCCESS | LHTTP_STATIC_ERR
 */
int http_request_free(http_request_t *req);

#endif