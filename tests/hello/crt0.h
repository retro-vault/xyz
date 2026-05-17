/*
 * Declares the tiny runtime symbols exported by the standalone
 * `tests/hello` startup code.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 Tomaz Stih
 */
#ifndef TESTS_HELLO_CRT0_H
#define TESTS_HELLO_CRT0_H

/*
 * Query a named runtime service table.
 */
extern void *query_service(char *name);
/*
 * Compatibility alias for `query_service()`.
 */
extern void *query_interface(char *name);

#endif /* TESTS_HELLO_CRT0_H */
