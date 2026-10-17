#include "http_request.h"
#include "http_response.h"

static int failures = 0;

#define CHECK(cond) do { \
    if (!(cond)) { \
        fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
        failures++; \
    } \
} while (0)

#define TEST(test) do { \
    fprintf(stdout, "------------------- %s -------------------\n", #test); \
    fprintf(stdout, "%s:%d: %s: test RUNNING\n", __FILE__, __LINE__, #test); \
    int f = failures; \
    test; \
    if (f != failures) { \
        fprintf(stderr, "%s:%d: %s: test FAILED\n", __FILE__, __LINE__, #test); \
    } else { \
        fprintf(stdout, "%s:%d: %s: test SUCCESS\n", __FILE__, __LINE__, #test); \
    } \
    fprintf(stdout, "------------------------------------\n"); \
} while (0)

static int test_simple_get(void) {
    int local_failures = failures;

    const char raw[] =
        "GET /index.html HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Accept: text/html\r\n"
        "\r\n";

    http_request_t req = {0};

    int res = http_request_from_data(&req, raw, sizeof(raw) - 1);
    CHECK(res == LHTTP_SUCCESS);
    if (res != LHTTP_SUCCESS) return -1;

    CHECK(req.method.num == HTTP_METHOD_GET);
    CHECK(req.version.num == HTTP_VERSION_1_1);
    CHECK(req.path.path != NULL && strcmp(req.path.path, "/index.html") == 0);
    CHECK(req.headers.len == 2);
    CHECK(http_headers_arr_find(&req.headers, "Host") >= 0);

    http_request_free(&req);

    return (local_failures == failures) ? 0 : -1;
}

static int test_simple_post(void) {
    int local_failures = failures;

    const char raw[] = 
        "POST /index.html HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Content-Length: 26\r\n"
        "Content-Type: application/json\r\n"
        "\r\n"
        "{\"message\": \"Hello World\"}"
        "\r\n";

    http_request_t req = {0};

    int res = http_request_from_data(&req, raw, sizeof(raw) - 1);
    CHECK(res == LHTTP_SUCCESS);
    if (res != LHTTP_SUCCESS) return -1;

    CHECK(req.method.num == HTTP_METHOD_POST);
    CHECK(req.version.num == HTTP_VERSION_1_1);
    CHECK(req.path.path != NULL && strcmp(req.path.path, "/index.html") == 0);
    CHECK(req.headers.len == 3);
    CHECK(http_headers_arr_find(&req.headers, "Host") >= 0);
    CHECK(http_headers_arr_find(&req.headers, "Content-Length") >= 0);
    CHECK(http_headers_arr_find(&req.headers, "Content-Type") >= 0);
    CHECK(req.body.data != NULL && strcmp(req.body.data, "{\"message\": \"Hello World\"}") == 0);

    http_request_free(&req);

    return (local_failures == failures) ? 0 : -1;
}

static int test_incomplete_request(void) {
    int local_failures = failures;

    const char raw[] = "GET / HTTP/1.1\r\nHost: example.com\r\n";

    http_request_t req = {0};

    CHECK(http_request_from_data(&req, raw, sizeof(raw) - 1) == LHTTP_REQUEST_INCOMPLETE);

    return (local_failures == failures) ? 0 : -1;
}

static int test_bytes_read(void) {
    int local_failures = failures;

    const char headers[] =
        "POST /a HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Content-Length: 5\r\n"
        "\r\n";
    const char raw[] =
        "POST /a HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "hello"
        "GET /b HTTP/1.1\r\n"
        "\r\n";

    const size_t headers_len = sizeof(headers) - 1;
    const size_t first_len = headers_len + 5;

    http_request_t req = {0};

    // Headers received, body only partially received
    CHECK(http_request_from_data(&req, raw, headers_len + 2) == LHTTP_REQUEST_INCOMPLETE);
    CHECK(req.bytes_parsed == headers_len);

    // Call again with the rest of the data
    CHECK(http_request_from_data(&req, raw, sizeof(raw) - 1) == LHTTP_SUCCESS);
    CHECK(req.bytes_parsed == first_len);
    CHECK(req.headers.len == 2);
    CHECK(strcmp(req.headers.headers[0].val, "example.com") == 0);
    CHECK(req.body.data != NULL && strcmp(req.body.data, "hello") == 0);

    http_request_free(&req);

    // Pipelined request starts where the first one ended
    http_request_t req2 = {0};

    CHECK(http_request_from_data(&req2, raw + first_len, sizeof(raw) - 1 - first_len) == LHTTP_SUCCESS);
    CHECK(req2.method.num == HTTP_METHOD_GET);
    CHECK(req2.path.path != NULL && strcmp(req2.path.path, "/b") == 0);
    CHECK(req2.bytes_parsed == sizeof(raw) - 1 - first_len);

    http_request_free(&req2);

    return (local_failures == failures) ? 0 : -1;
}

static int test_malformed_request(void) {
    int local_failures = failures;

    // First line missing path and version
    const char bad_first_line[] =
        "GARBAGE\r\n"
        "Host: example.com\r\n"
        "\r\n";

    // Unknown method
    const char bad_method[] =
        "FOO / HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "\r\n";

    // Negative Content-Length
    const char bad_content_length[] =
        "POST / HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Content-Length: -5\r\n"
        "\r\n"
        "hello";

    http_request_t req = {0};

    CHECK(http_request_from_data(&req, bad_first_line, sizeof(bad_first_line) - 1) == LHTTP_STR_MALFORMED);

    req = (http_request_t) {0};
    CHECK(http_request_from_data(&req, bad_method, sizeof(bad_method) - 1) == LHTTP_UNKOWN_METHOD);

    req = (http_request_t) {0};
    CHECK(http_request_from_data(&req, bad_content_length, sizeof(bad_content_length) - 1) == LHTTP_REQUEST_MALFORMED);
    http_request_free(&req);

    return (local_failures == failures) ? 0 : -1;
}

static int test_response(void) {
    http_response_t resp = {0};

    int local_failures = failures;

    char ct[] = "Content-Type: application/json";
    char cl[] = "Content-Length: 26";
    char h[]  = "Host: www.ltale.com";
    char b[]  = "{\"message\": \"Hello World\"}";
    const char expected[] = "HTTP/1.1 200 OK\r\n"
                            "Content-Type: application/json\r\n"
                            "Content-Length: 26\r\n"
                            "Host: www.ltale.com\r\n"
                            "\r\n"
                            "{\"message\": \"Hello World\"}";

    const char *data_out = NULL;
    size_t size = 0;

    CHECK(http_version_str_to_v(&resp.version, "HTTP/1.1") == LHTTP_SUCCESS);
    CHECK(http_response_code_parse_num(&resp.response_code, 200) == LHTTP_SUCCESS);

    CHECK(http_headers_arr_append_str(&resp.headers, ct, sizeof(ct) - 1) == LHTTP_SUCCESS);
    CHECK(http_headers_arr_append_str(&resp.headers, cl, sizeof(cl) - 1) == LHTTP_SUCCESS);
    CHECK(http_headers_arr_append_str(&resp.headers, h, sizeof(h) - 1) == LHTTP_SUCCESS);

    CHECK(http_body_initialize(&resp.body, b, sizeof(b) - 1) == LHTTP_SUCCESS);

    CHECK(http_response_to_data(&resp, &data_out, &size) == LHTTP_SUCCESS);
    CHECK(data_out != NULL && strcmp(data_out, expected) == 0);
    CHECK(size == sizeof(expected) - 1);

    free((void *) data_out);
    http_response_free(&resp);

    return (local_failures == failures) ? 0 : -1;
}

int main(void) {
    TEST(test_simple_get());
    TEST(test_incomplete_request());
    TEST(test_simple_post());
    TEST(test_bytes_read());
    TEST(test_malformed_request());
    TEST(test_response());

    return failures == 0 ? 0 : 1;
}
