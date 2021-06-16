#define MSB_SET(x) ((x >> (8*sizeof(x)-1)) & 1)

unsigned long long
_modulonglong (unsigned long long a, unsigned long long b)
{
  unsigned char count = 0;

  while (!MSB_SET(b))
  {
     b <<= 1;
     if (b > a)
     {
        b >>=1;
        break;
     }
     count++;
  }
  do
  {
    if (a >= b)
      a -= b;
    b >>= 1;
  }
  while (count--);

  return a;
}