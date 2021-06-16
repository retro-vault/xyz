long
_divslong (long x, long y)
{
  long r;

  r = (unsigned long)(x < 0 ? -x : x) / (unsigned long)(y < 0 ? -y : y);
  if ((x < 0) ^ (y < 0))
    return -r;
  else
    return r;
}
