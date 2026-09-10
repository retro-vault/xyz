/*
 * Declares the low-level Kempston mouse polling helpers.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2021 tomaz stih
 */
#ifndef __KEMPSTON_H__
#define __KEMPSTON_H__

#include <stdint.h>

/*
 * Set the current mouse origin or calibration point.
 */
extern void mouse_calibrate(uint8_t x, uint8_t y);
/*
 * Poll the Kempston mouse and store the latest state.
 */
extern void _mouse_scan(void *mouse_info);
#define __mouse_scan _mouse_scan

#endif /* __KEMPSTON_H__ */
