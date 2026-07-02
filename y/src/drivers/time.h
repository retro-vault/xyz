/*
 * Declares the small YOS-compatible time interface.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2021 tomaz stih
 */
#ifndef __TIME_H__
#define __TIME_H__

/*
 * Number of scheduler ticks per second.
 */
#define CLOCKS_PER_SEC  50
/*
 * Clock tick counter type.
 */
typedef unsigned int clock_t;

/*
 * Return the current system tick counter.
 */
extern clock_t __clock(void);
extern void __clock_tick(void);

#define clock __clock
#define _clock __clock
#define _clock_tick __clock_tick

#endif /* __TIME_H__ */
