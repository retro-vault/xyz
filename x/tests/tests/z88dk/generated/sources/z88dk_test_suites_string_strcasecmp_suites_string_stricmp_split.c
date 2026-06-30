/* auto-generated z88dk string split */





#include "string_tests.h"

static int (*func)(const char *x,const  char *y) [[z88dk::smallc]];

void stricmp_equal_lower()
{
    Assert(strcasecmp("equal","equal") == 0, "Should be == 0");
}


void stricmp_equal_upper()
{
    Assert(strcasecmp("EQUAL","EQUAL") == 0, "Should be == 0");
}


void stricmp_equal_mixed()
{
    Assert(strcasecmp("EqUaL","eQuAl") == 0, "Should be == 0");
}


void stricmp_less()
{
    Assert(strcasecmp("EQUAL","equam") < 0, "Should be < 0");
}


void stricmp_greater()
{
    Assert(strcasecmp("equam","EQUAL") > 0, "Should be > 0");
}



int main(void)
{
    suite_setup("Strcasecmp Tests");
    suite_add_test(stricmp_equal_lower);
    suite_add_test(stricmp_equal_upper);
    suite_add_test(stricmp_equal_mixed);
    suite_add_test(stricmp_less);
    suite_add_test(stricmp_greater);
    return suite_run();
}
