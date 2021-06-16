long long _mullonglong(long long ll, long long lr)
{
  unsigned long long ret = 0ull;
  unsigned char i, j;

  for (i = 0; i < sizeof (long long); i++)
    {
      unsigned char l = ll >> (i * 8);
      for(j = 0; (i + j) < sizeof (long long); j++)
        {
          unsigned char r = lr >> (j * 8);
          ret += (unsigned long long)((unsigned short)(l * r)) << ((i + j) * 8);
        }
    }

  return(ret);
}