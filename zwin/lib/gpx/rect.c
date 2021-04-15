/*
 * rect.c
 *
 * rectangle arith. functions
 *
 * NOTES:
 *  this is pure "C" implementation and it might be a bit non-optimal for
 *  8-bit machines. consider writing parts of it in assembly.
 * 
 * MIT License (see: LICENSE)
 * copyright (c) 2021 tomaz stih
 *
 * 07.10.2013   tstih
 *
 */
#include "rect.h"

boolean_t rect_contains(rect_t *r, coord_t x, coord_t y) {
	return (r->x0 <= x && r->x1 >= x && r->y0 <= y && r->y1 >=y );
}

boolean_t rect_overlap(rect_t *a, rect_t *b) {
	return !(a->y1 < b->y0 || a->y0 > b->y1 || a->x1 < b->x0 || a->x0 > b->x1);
}

rect_t* rect_intersect(rect_t *a, rect_t *b, rect_t *intersect) {
	if (rect_overlap(a,b)) {
		intersect->x0=MAX(a->x0,b->x0);
		intersect->y0=MAX(a->y0,b->y0);
		intersect->x1=MIN(a->x1,b->x1);
		intersect->y1=MIN(a->y1,b->y1);
		return intersect;
	} else return NULL;
}

rect_t* rect_inflate(rect_t* rect, coord_t dx, coord_t dy) {
	rect->x0-=dx;
	rect->x1+=dx;
	rect->y0-=dy;
	rect->y1+=dy;
}

rect_t* rect_rel2abs(rect_t* abs, rect_t* rel, rect_t* out) {
	out->x0=abs->x0+rel->x0;
	out->x1=abs->x1+rel->x1;
	out->y0=abs->y0+rel->y0;
	out->y1=abs->y1+rel->y1;
}

void rect_subtract(
	rect_t *outer, 		/* outer rect */
	rect_t *inner, 		/* inner rect */
	rect_t *result,		/* rect array (4!) */
	byte_t *num) {		/* returns actual rects in result */

	*num = 0;		/* assume */
	if (outer->y0 < inner->y0) {
		result->x0=outer->x0;
		result->y0=outer->y0;
		result->x1=outer->x1;
		result->y1=inner->y0-1;
		(*num)++;
		result++;	
	}	
	if (inner->y1 < outer->y1) {
		result->x0=outer->x0;
		result->y0=inner->y1+1;
		result->x1=outer->x1;
		result->y1=outer->y1;
		(*num)++;
		result++;		
	}
	if (outer->x0 < inner->x0) {
		result->x0=outer->x0;
		result->y0=inner->y0;
		result->x1=inner->x0-1;
		result->y1=inner->y1;
		(*num)++;
		result++;
	}
	if (inner->x1 < outer->x1) {
		result->x0=inner->x1+1;
		result->y0=inner->y0;
		result->x1=outer->x1;
		result->y1=inner->y1;
		(*num)++;
		result++;
	}
}

/* TODO: careful at scren edges */
void rect_delta_offset(rect_t *rect, coord_t oldx, coord_t newx, coord_t oldy, coord_t newy, coord_t size_only) {
	if (oldx > newx) { /* move left */
		if (!size_only) rect->x0 = rect->x0 - (oldx-newx);
		rect->x1 = rect->x1 - (oldx-newx);
	} else { /* move right */
		if (!size_only) rect->x0 = rect->x0 + (newx-oldx);
		rect->x1 = rect->x1 + (newx-oldx);
	}

	if (oldy > newy) { /* move up */
		if (!size_only) rect->y0 = rect->y0 - (oldy-newy);
		rect->y1 = rect->y1 - (oldy-newy);
	} else { /* move down */
		if (!size_only) rect->y0 = rect->y0 + (newy-oldy);
		rect->y1 = rect->y1 + (newy-oldy);
	}
}
