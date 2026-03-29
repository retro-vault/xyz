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

/* interrupt enable and disable with refcounting */
extern void ir_enable(void);
extern void ir_disable(void);

#endif /* __INTERRUPTS_H__ */