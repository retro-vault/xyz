/*
 * Declares the reset and interrupt-vector table helpers used by YOS.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2021 tomaz stih
 */
#ifndef __VECTORS_H__
#define __VECTORS_H__

#include <stdint.h>

/*
 * Reset vector index for `RST 08h`.
 */
#define	RST08   0
/*
 * Reset vector index for `RST 10h`.
 */
#define	RST10   1
/*
 * Reset vector index for `RST 18h`.
 */
#define	RST18   2
/*
 * Reset vector index for `RST 20h`.
 */
#define	RST20   3
/*
 * Reset vector index for `RST 28h`.
 */
#define	RST28   4
/*
 * Reset vector index for `RST 30h`.
 */
#define	RST30   5
/*
 * Reset vector index for `RST 38h`.
 */
#define	RST38   6
/*
 * Interrupt vector index for the non-maskable interrupt.
 */
#define NMI	    7

/*
 * Install one handler into the selected vector slot.
 */
extern void sys_vec_set(void (*handler)(void), uint8_t vec_num);

/*
 * Return the current handler stored in the selected vector slot.
 */
extern void *sys_vec_get(uint8_t vec_num);

#endif /* __VECTORS_H__ */
