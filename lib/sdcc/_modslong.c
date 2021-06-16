long
_modslong (long a, long b)
{
  long r;

  r = (unsigned long)(a < 0 ? -a : a) % (unsigned long)(b < 0 ? -b : b);

  if (a < 0)
    return -r;
  else
    return r;
}
