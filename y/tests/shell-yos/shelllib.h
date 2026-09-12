/*
 * Self-registering library fixture shared by the shell and kernel tests.
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#ifndef SHELLLIB_H
#define SHELLLIB_H

#include <stdint.h>

typedef struct shelllib_api {
    uint16_t (*probe)(void);
    const char *(*message)(void);
    uint16_t (*initializations)(void);
    uint16_t (*calls)(void);
} shelllib_api_t;

#define SHELLLIB_RESULT 0x600d

#endif
