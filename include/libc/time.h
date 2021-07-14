/*
 * time.h
 *
 * Standard C header file for time functions.
 * Implemented functions are: clock()
 * 
 * MIT License (see: LICENSE)
 * copyright (c) 2021 tomaz stih
 *
 * 22.05.2021   tstih
 *
 */
#ifndef __TIME_H__
#define __TIME_H__

/* zx spectrum clock has a resolution of 1/50 sec */
#define CLOCKS_PER_SEC  50
typedef unsigned int clock_t;

/* Return current clock in 1/1000 seconds */
extern clock_t clock(void);

#endif /* __TIME_H__ */