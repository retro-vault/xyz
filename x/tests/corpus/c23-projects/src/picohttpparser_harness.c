#include <string.h>

#include "picohttpparser.h"

int main(void) {
    const char *req = "GET /z80 HTTP/1.1\r\nHost: x\r\n\r\n";
    const char *method = 0;
    const char *path = 0;
    size_t method_len = 0;
    size_t path_len = 0;
    int minor = 0;
    struct phr_header headers[4];
    size_t num_headers = 4;
    int n = phr_parse_request(req, strlen(req), &method, &method_len, &path,
                              &path_len, &minor, headers, &num_headers, 0);

    return n == (int)strlen(req) && method_len == 3 &&
           strncmp(method, "GET", 3) == 0 && path_len == 4 &&
           strncmp(path, "/z80", 4) == 0 && minor == 1 &&
           num_headers == 1 ? 0 : 1;
}
