/* bug-3969.c

   The warning about enumeration constnats not fitting into an it was emitted even in C23 mode (despite it no longer being a constraint violation in C23)
 */

#ifdef TEST1
#pragma std_c11
enum { BLEH_ADDR = 0xFFF4}; /* WARNING */
#endif

#ifdef TEST1
#pragma std_c23
enum { BLEH_ADDR = 0xFFF4};
#endif

