/* hello-yos.c
 *
 * Minimal user-space XL program:
 *   - fetches the "yos" service via RST10 bridge
 *   - prints hello world using yos->printf
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 Tomaz Stih
 */

#include <yos.h>
#include "../../../x/tests/tests/hello/crt0.h"

int main(void) {
    yos_t *y = (yos_t*)query_service("yos");
    if (!y) return 1;
    y->printf("HELLO WORLD\n");
    return 0;
}
