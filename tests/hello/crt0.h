/* crt0.h
 *
 * Minimal runtime API exported by tests/hello/crt0.s
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 Tomaz Stih
 */
#ifndef TESTS_HELLO_CRT0_H
#define TESTS_HELLO_CRT0_H

extern void *query_service(char *name);
extern void *query_interface(char *name); /* compatibility alias */

#endif /* TESTS_HELLO_CRT0_H */
