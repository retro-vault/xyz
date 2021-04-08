/*
 *	rect.h
 *	rectangle type
 *
 *	tomaz stih mon oct 7 2013
 */
#ifndef _RECT_H
#define _RECT_H

#include "yos.h"

typedef struct rect_s rect_t;
struct rect_s {
	coord x0;
	coord y0;
	coord x1;
	coord y1;
};

extern boolean rect_contains(rect_t *r, coord x, coord y);
extern boolean rect_overlap(rect_t *a, rect_t *b);
extern rect_t* rect_inflate(rect_t* rect, coord dx, coord dy);
extern rect_t* rect_intersect(rect_t *a, rect_t *b, rect_t *intersect);
extern rect_t* rect_rel2abs(rect_t* abs, rect_t* rel, rect_t* out);
extern void rect_subtract(rect_t *outer, rect_t *inner, rect_t *result,	byte *num);
extern void rect_delta_offset(rect_t *rect, coord oldx, coord newx, coord oldy, coord newy,  coord size_only);

#endif /* _RECT_H */
