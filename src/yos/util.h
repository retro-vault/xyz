/*
 * util.h
 *
 * misc utility functions
 * 
 * MIT License (see: LICENSE)
 * copyright (C) 2021 tomaz stih
 *
 * 2021-07-12   tstih
 *
 */
#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdint.h>

/* get low byte of word */
extern uint8_t lob(uint16_t w);

/* get high byte of word */
extern uint8_t hib(uint16_t w); 

#endif /* __UTIL_H__ */