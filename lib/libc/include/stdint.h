/*
 * Declares the fixed-width integer types used by the tiny libc subset.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2021 tomaz stih
 */
#ifndef __STDINT_H__
#define __STDINT_H__

/*
 * Signed 8-bit integer type.
 */
typedef char            int8_t;
/*
 * Unsigned 8-bit integer type.
 */
typedef unsigned char   uint8_t;
/*
 * Signed 16-bit integer type.
 */
typedef int             int16_t;
/*
 * Unsigned 16-bit integer type.
 */
typedef unsigned int    uint16_t;
/*
 * Signed 32-bit integer type.
 */
typedef long            int32_t;
/*
 * Unsigned 32-bit integer type.
 */
typedef unsigned long   uint32_t;

#endif /* __STDINT_H_ */
