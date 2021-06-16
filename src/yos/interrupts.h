/*
 * interrupts.h
 *
 * interrupt handling
 * 
 * MIT License (see: LICENSE)
 * copyright (C) 2021 tomaz stih
 *
 * 2021-06-16   tstih
 *
 */
#ifndef __INTERRUPTS_H__
#define __INTERRUPTS_H__

#include <stdint.h>

#define	RST08   0
#define	RST10   1
#define	RST18   2
#define	RST20   3
#define	RST28   4
#define	RST30   5
#define	RST38   6
#define NMI	    7

/* interrupt enable and disable with refcounting */
extern void ir_enable();
extern void ir_disable();

#endif /* __INTERRUPTS_H__ */