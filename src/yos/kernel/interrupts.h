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

/* refcounted critical sections */
extern void enter_critical_section(void);
extern void leave_critical_section(void);

#endif /* __INTERRUPTS_H__ */
