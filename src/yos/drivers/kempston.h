/*
 * kempston.h
 *
 * kempston mouse driver
 * 
 * MIT License (see: LICENSE)
 * copyright (c) 2021 tomaz stih
 *
 * 27.07.2021   tstih
 *
 */
#ifndef __KEMPSTON_H__
#define __KEMPSTON_H__

#include <stdint.h>

extern void mouse_calibrate(uint8_t x, uint8_t y);
extern void _mouse_scan(void *mouse_info);

#endif /* __KEMPSTON_H__ */