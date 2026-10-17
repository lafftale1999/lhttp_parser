#include "http_version.h"

static const http_version_t version_map[] = {
    {.num = HTTP_VERSION_1_1, .str = "1.1"},
};

static const size_t version_map_size = sizeof(version_map) / sizeof(version_map[0]);

static const char TRIAL_VERSION[] = "1.1";
static const char LEADING_V_STR[] = "HTTP/";

static int http_version_boundary_check(const char *str) {
    if (str == NULL) {
        fprintf(stderr, "http_version_boundary_check failed, str is NULL\n");
        return LHTTP_STATIC_ERR;
    }

    size_t size = 0;

    while (size++ < sizeof(TRIAL_VERSION)) {
        if (str[size - 1] == '\0') return 0;
    }

    return -1;
}

int http_version_str_to_v(http_version_t *out, const char *str) {
    size_t offset = sizeof(LEADING_V_STR) - 1;
    const char *version;

    if (str == NULL) {
        fprintf(stderr, "http_version_str_to_v failed, str is NULL\n");
        return LHTTP_STATIC_ERR;
    }

    if (!strncmp(LEADING_V_STR, str, offset)) {
        version = str + offset;
    } else {
        version = str;
    }

    if (http_version_boundary_check(version) != 0) {
        fprintf(stderr, "http_version_str_to_v failed, string is malformed\n");
        return LHTTP_STR_MALFORMED;
    }

    for (size_t idx = 0; idx < version_map_size; idx++) {
        const http_version_t *tmp = &version_map[idx];

        if (!strcmp(version, tmp->str)) {
            memcpy(out, tmp, sizeof(*tmp));
            return LHTTP_SUCCESS;
        }
    }

    fprintf(stderr, "http_version_str_to_v failed, unkown version\n");
    return LHTTP_UNKOWN_VERSION;
}