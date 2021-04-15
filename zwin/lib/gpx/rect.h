/*
 * rect.h
 *
 * rectangle arith. functions
 *
 * MIT License (see: LICENSE)
 * copyright (c) 2021 tomaz stih
 *
 * 07.10.2013   tstih
 *
 */
#ifndef _RECT_H
#define _RECT_H

#include "gpx.h"

extern boolean_t rect_contains(rect_t *r, coord_t x, coord_t y);
extern boolean_t rect_overlap(rect_t *a, rect_t *b);
extern rect_t* rect_inflate(rect_t* rect, coord_t dx, coord_t dy);
extern rect_t* rect_intersect(rect_t *a, rect_t *b, rect_t *intersect);
extern rect_t* rect_rel2abs(rect_t* abs, rect_t* rel, rect_t* out);
extern void rect_subtract(rect_t *outer, rect_t *inner, rect_t *result,	byte_t *num);
extern void rect_delta_offset(rect_t *rect, coord_t oldx, coord_t newx, coord_t oldy, coord_t newy,  coord_t size_only);

#endif /* _RECT_H */
