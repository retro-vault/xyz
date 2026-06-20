#include <stdbool.h>
#include <string.h>

#include "cwalk.h"

int main(void) {
    char out[80];
    const char *base = 0;
    const char *ext = 0;
    struct cwk_segment segment;
    size_t len = 0;

    cwk_path_set_style(CWK_STYLE_UNIX);

    cwk_path_get_basename("/opt/x/tools/xcc.bin", &base, &len);
    if (!base) return 5;
    if (len != 7) return 20 + (int)len;
    if (strncmp(base, "xcc.bin", len) != 0) return 6;

    if (!cwk_path_get_extension("/opt/x/tools/xcc.bin", &ext, &len)) return 7;
    if (len != 4 || strncmp(ext, ".bin", len) != 0) return 8;

    if (!cwk_path_get_first_segment("/opt/x/tools", &segment)) return 11;
    if (segment.size != 3 || strncmp(segment.begin, "opt", 3) != 0) return 12;
    if (!cwk_path_get_next_segment(&segment)) return 13;
    if (segment.size != 1 || strncmp(segment.begin, "x", 1) != 0) return 14;
    if (cwk_path_get_segment_type(&segment) != CWK_NORMAL) return 15;

    if (!cwk_path_is_absolute("/opt/x")) return 16;
    if (!cwk_path_is_relative("opt/x")) return 17;
    if (!cwk_path_has_extension("/opt/x/tools/xcc.bin")) return 18;
    (void)out;
    return 0;
}
