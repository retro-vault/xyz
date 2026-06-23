/** tests for strsep
*/

#include <testfwk.h>

#include <string.h>

const char teststring[] = "hello!,hello!,goodbye!!";
char buffer[24];

void
testStr (void)
{
#if  defined (__SDCC) || (_POSIX_C_SOURCE >= 200809L) // strnlen is an SDCC extension available in POSIX.
  char *ptr;
  char *oldptr;

  strcpy (buffer, teststring);
  ptr = buffer;
  oldptr = strsep (&ptr, "!");
  ASSERT (oldptr == buffer);
  ASSERT (!strcmp (oldptr, "hello"));
  oldptr = strsep (&ptr, "!");
  ASSERT (!strcmp (oldptr, ",hello"));
  oldptr = strsep (&ptr, "!");
  ASSERT (!strcmp (oldptr, ",goodbye"));
  oldptr = strsep (&ptr, "!");
  ASSERT (!strcmp (oldptr, ""));
  oldptr = strsep (&ptr, "!");
  ASSERT (!strcmp (oldptr, ""));

  strcpy (buffer, teststring);
  ptr = buffer;
  oldptr = strsep (&ptr, ",");
  ASSERT (oldptr == buffer);
  ASSERT (!strcmp (oldptr, "hello!"));
  oldptr = strsep (&ptr, ",");
  ASSERT (!strcmp (oldptr, "hello!"));
  oldptr = strsep (&ptr, ",");
  ASSERT (!strcmp (oldptr, "goodbye!!"));

  strcpy (buffer, teststring);
  ptr = buffer;
  oldptr = strsep (&ptr, "!,");
  ASSERT (oldptr == buffer);
  ASSERT (!strcmp (oldptr, "hello"));
  oldptr = strsep (&ptr, "!,");
  ASSERT (!strcmp (oldptr, ""));
  oldptr = strsep (&ptr, "!,");
  ASSERT (!strcmp (oldptr, "hello"));
  oldptr = strsep (&ptr, "!,");
  ASSERT (!strcmp (oldptr, ""));
  oldptr = strsep (&ptr, "!,");
  ASSERT (!strcmp (oldptr, "goodbye"));
#endif
}

