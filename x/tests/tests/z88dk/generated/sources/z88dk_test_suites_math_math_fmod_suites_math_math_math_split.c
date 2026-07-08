/* auto-generated z88dk math split */
#define GENMATH 1
#define MATH_LIBRARY "Genmath"
#define suite_math z88dk_test_suites_math_math_fmod_orig_suite
#define main z88dk_test_suites_math_math_fmod_orig_main
#include "../../upstream/test/suites/math/math.c"
#undef main
#undef suite_math

int main(void)
{
    suite_setup(MATH_LIBRARY " Tests");
    suite_add_test(test_fmod);
    return suite_run();
}
