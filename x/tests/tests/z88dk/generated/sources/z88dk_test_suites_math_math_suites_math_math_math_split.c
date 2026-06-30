/* auto-generated z88dk math split */
#define GENMATH 1
#define MATH_LIBRARY "Genmath"
#define suite_math z88dk_test_suites_math_math_orig_suite
#define main z88dk_test_suites_math_math_orig_main
#include "../../upstream/test/suites/math/math.c"
#undef main
#undef suite_math

int main(void)
{
    suite_setup(MATH_LIBRARY " Tests");
    suite_add_test(test_comparison);
    suite_add_test(test_integer_operations);
    suite_add_test(test_integer_constant_operations);
    suite_add_test(test_integer_constant_longform);
    suite_add_test(test_integer_constant_longform_lhs);
    suite_add_test(test_post_incdecrement);
    suite_add_test(test_pre_incdecrement);
    suite_add_test(test_approx_equal);
    suite_add_test(test_sqrt);
    suite_add_test(test_pow);
    suite_add_test(test_fmin);
    suite_add_test(test_fmax);
    return suite_run();
}
