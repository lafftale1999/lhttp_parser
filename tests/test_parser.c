#include "http_parser.h"

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

    int res = http_parser_data_to_request(&req, raw, sizeof(raw) - 1);
    CHECK(res == LHTTP_SUCCESS);
    if (res != LHTTP_SUCCESS) return -1;

    CHECK(req.method == HTTP_METHOD_GET);
    CHECK(req.version == HTTP_VERSION_1_1);
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

    int res = http_parser_data_to_request(&req, raw, sizeof(raw) - 1);
    CHECK(res == LHTTP_SUCCESS);
    if (res != LHTTP_SUCCESS) return -1;

    CHECK(req.method == HTTP_METHOD_POST);
    CHECK(req.version == HTTP_VERSION_1_1);
    CHECK(req.path.path != NULL && strcmp(req.path.path, "/index.html") == 0);
    CHECK(req.headers.len == 3);
    CHECK(http_headers_arr_find(&req.headers, "Host") >= 0);
    CHECK(http_headers_arr_find(&req.headers, "Content-Length") >= 0);
    CHECK(http_headers_arr_find(&req.headers, "Content-Type") >= 0);
    CHECK(req.body.data != NULL && strcmp(req.body.data, "{\"message\": \"Hello World\"}") == 0);

    return (local_failures == failures) ? 0 : -1;
}

static int test_incomplete_request(void) {
    int local_failures = failures;

    const char raw[] = "GET / HTTP/1.1\r\nHost: example.com\r\n";

    http_request_t req = {0};

    CHECK(http_parser_data_to_request(&req, raw, sizeof(raw) - 1) == LHTTP_REQUEST_INCOMPLETE);

    return (local_failures == failures) ? 0 : -1;
}

int main(void) {
    TEST(test_simple_get());
    TEST(test_incomplete_request());
    TEST(test_simple_post());

    return failures == 0 ? 0 : 1;
}
