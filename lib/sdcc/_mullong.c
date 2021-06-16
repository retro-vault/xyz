/* little endian order */
union bil {
        struct {unsigned char b0,b1,b2,b3 ;} b;
        struct {unsigned short lo,hi ;} i;
        unsigned long l;
        struct { unsigned char b0; unsigned short i12; unsigned char b3;} bi;
} ;

#define bcast(x) ((union bil*)&(x))

long
_mullong (long a, long b)
{
  unsigned short i12;

  bcast(a)->i.hi *= bcast(b)->i.lo;
  bcast(a)->i.hi += bcast(b)->i.hi * bcast(a)->i.lo;

  /* only (a->i.lo * b->i.lo) 16x16->32 to do. asm? */
  bcast(a)->i.hi += bcast(a)->b.b1 * bcast(b)->b.b1;

  i12 = bcast(b)->b.b0 * bcast(a)->b.b1;
  bcast(b)->bi.i12 = bcast(a)->b.b0 * bcast(b)->b.b1;

  /* add up the two partial result, store carry in b3 */
  bcast(b)->b.b3 = ((bcast(b)->bi.i12 += i12) < i12);

  bcast(a)->i.lo  = bcast(a)->b.b0 * bcast(b)->b.b0;

  bcast(b)->bi.b0 = 0;

  return a + b;
}