# lhttp_parser
A lightweight HTTP parser written in C11, capable of parsing requests and creating correctly
formatted responses. It is **not** suitable for servers exposed to the internet and should only
be used within trusted environments.

Currently only `HTTP/1.1` is supported.

## Getting started
Follow these steps to use this in your project:

### 1. Prerequisites
Ensure that you have all the build tools available:
* `make`
* `gcc`

```sh
sudo apt update
sudo apt install build-essential
```

>Note: **gdb** can be useful for debugging purposes.

### 2. Download
To include this in your project:

* **Offline** - If your build environment does not have access to the internet, `lhttp_parser`
can be downloaded and included without any external dependencies (besides the ones stated above).

* **Online** - If your build environment has access to the internet, best practice is to include
it as a `submodule`.

    ```sh
    git submodule add https://github.com/lafftale1999/lhttp_parser.git path/to/lhttp_parser
    git submodule update --init --recursive
    ```

### 3. Include
To include this in your `Makefile`, include `http_parser.mk` and pick one of the options below:

```Makefile
include path/to/lhttp_parser/http_parser.mk

...

# Option A - link the static library
CFLAGS += $(LAFFTALE_HTTP_LIB_INC)
app: $(OBJS) $(LAFFTALE_HTTP_LIB_A)
	$(CC) -o $@ $^

# Option B - compile the sources straight into your project
SRCS   += $(LAFFTALE_HTTP_LIB_SRCS)
CFLAGS += $(LAFFTALE_HTTP_LIB_INC)
```

You may also set the following variables **before** the `include` in your `Makefile`:

| Variable | Description | Default |
|---|---|---|
| `LAFFTALE_HTTP_LIB_BUILD_DIR` | Where objects and the library go | `<lib>/build` |
| `LAFFTALE_HTTP_LIB_CONFIG_DIR` | Directory holding your own `http_config.h` | `<lib>` |
| `LAFFTALE_HTTP_LIB_CFLAGS` | Flags used when building the library | `-std=c11 -Wall -Wextra -O2` |

### Configuration
Limits are set in `http_config.h`. To override them, copy the file into your own directory and point
`LAFFTALE_HTTP_LIB_CONFIG_DIR` to it.

| Define | Description | Default |
|---|---|---|
| `LHTTP_HEADER_MAX_SIZE` | Max size of a single header | `2048` |
| `LHTTP_HEADER_ARR_INIT_SIZE` | Initial capacity of the header array | `10` |
| `LHTTP_HEADER_ARR_MODIFIER` | Growth factor when the header array is full | `2` |

## Building and testing
The library can also be built standalone from its root directory:

```sh
make            # build build/libhttp_parser.a
make test       # build and run every tests/*.c
make DEBUG=1    # build with -g -O0 and address/undefined sanitizers
make clean
```

## Guide
Below is a simple guide on how to use the API for requests and responses.

### Including
To make use of the API in `lhttp_parser`, include the header file `lhttp_parser.h` where needed.

### Errors
All error codes returned by the library are defined in `include/common/http_err.h`. Every function
returns `LHTTP_SUCCESS` (`0`) on success and a negative value on failure.

>Note: *These are not HTTP status codes, but describe why a function failed so that it can be
handled logically.*

### Example: parsing an incoming request

`main.c`
```c
#include "lhttp_parser.h"

int main(void) {
    int res;

    // Representing incoming data
    const char raw[] =
        "GET /index.html HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Accept: text/html\r\n"
        "\r\n";

    // Zero initialized
    http_request_t req = {0};

    // Parse request (adjustment for null-terminator)
    res = http_request_from_data(&req, raw, sizeof(raw) - 1);
    if (res == LHTTP_REQUEST_INCOMPLETE) {
        // Free request and try again
    } else if (res != LHTTP_SUCCESS) {
        // Handle errors
    }

    ...

    res = http_request_free(&req);
    if (res != LHTTP_SUCCESS) {
        // Handle errors
    }

    return 0;
}
```

### Example: building a response

`main.c`
```c
#include <stdlib.h>
#include "lhttp_parser.h"

int main(void) {
    int res;

    // Zero initialized
    http_response_t resp = {0};
    const char *data_out = NULL;
    size_t size = 0;

    // Status line: HTTP/1.1 200 OK
    if (http_version_str_to_v(&resp.version, "HTTP/1.1") != LHTTP_SUCCESS ||
        http_response_code_parse_num(&resp.response_code, 200) != LHTTP_SUCCESS) {
        // Handle errors
    }

    // Headers in string format
    // Notice that the Content-Length is hardcoded, this should be synced with the size of the body.
    char ct[] = "Content-Type: application/json";
    char cl[] = "Content-Length: 26";
    char b[]  = "{\"message\": \"Hello World\"}";

    // Initialize headers (adjustment for null-terminator)
    if (http_headers_arr_append_str(&resp.headers, ct, sizeof(ct) - 1) < 0 ||
        http_headers_arr_append_str(&resp.headers, cl, sizeof(cl) - 1) < 0) {
        // Handle errors
    }

    res = http_body_initialize(&resp.body, b, sizeof(b) - 1);
    if (res < 0) {
        // Handle errors
    }

    res = http_response_to_data(&resp, &data_out, &size);
    if (res < 0) {
        // Handle errors
    }

    // Response is now fully crafted
    // data_out holds `size` bytes and is ready to be sent.

    ...

    // data_out is allocated by the library and must be freed by the caller
    free((void *) data_out);
    http_response_free(&resp);

    return 0;
}
```
