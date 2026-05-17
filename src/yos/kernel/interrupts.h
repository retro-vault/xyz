/*
 * Declares the refcounted critical-section helpers used to guard shared
 * kernel data from interrupt-time mutation.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2021 tomaz stih
 */
#ifndef __INTERRUPTS_H__
#define __INTERRUPTS_H__

#include <stdint.h>

/*
 * Enter a refcounted critical section.
 */
extern void enter_critical_section(void);
/*
 * Leave a previously entered refcounted critical section.
 */
extern void leave_critical_section(void);

#endif /* __INTERRUPTS_H__ */
