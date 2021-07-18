/*
 * vectors.h
 *
 * vector handling
 * 
 * MIT License (see: LICENSE)
 * copyright (C) 2021 tomaz stih
 *
 * 2021-06-20   tstih
 *
 */
#ifndef __VECTORS_H__
#define __VECTORS_H__

#include <stdint.h>

#define	RST08   0
#define	RST10   1
#define	RST18   2
#define	RST20   3
#define	RST28   4
#define	RST30   5
#define	RST38   6
#define NMI	    7

/* set vector */
extern void sys_vec_set(void (*handler)(), uint8_t vec_num); 

/* get vector */
extern void *sys_vec_get(uint8_t vec_num);

#endif /* __VECTORS_H__ */