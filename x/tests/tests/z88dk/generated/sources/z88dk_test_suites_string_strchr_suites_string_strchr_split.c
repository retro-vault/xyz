/* auto-generated z88dk string split */


#include "string_tests.h"

void strchr_tests() 
{
   char *haystack = "needle";

   Assert(haystack + 1 == strchr(haystack,'e'), "Should find at position 1");
   Assert(NULL == strchr(haystack,'a'), "Should not find");
}


int main(void)
{
    suite_setup("str*chr Tests");
    suite_add_test(strchr_tests);
    return suite_run();
}
