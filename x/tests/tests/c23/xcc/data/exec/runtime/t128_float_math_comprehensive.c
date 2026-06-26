/*
 * Z80 C23 Float + math.h Comprehensive Test
 * Tests almost every aspect of floating-point support.
 */

#include <stdio.h>
#include <math.h>
#include <float.h>
#include <stddef.h>

#define EPSILON 1e-6
#define DOUBLE_EPSILON 1e-9

static int tests_passed = 0;
static int tests_total = 0;

#define TEST_ASSERT(cond, msg) do { \
    tests_total++; \
    if (cond) { \
        tests_passed++; \
        printf("  [PASS] %s\n", msg); \
    } else { \
        printf("  [FAIL] %s\n", msg); \
    } \
} while(0)

static double my_abs(double x) { return x < 0.0 ? -x : x; }

int main(void) {
    printf("=== Z80 C23 FLOAT + MATH.H TEST ===\n\n");

    printf("sizeof(float) = %zu, sizeof(double) = %zu\n", sizeof(float), sizeof(double));
    printf("FLT_EPSILON ≈ %e, DBL_EPSILON ≈ %e\n\n", FLT_EPSILON, DBL_EPSILON);

    /* Basic literals and operations */
    float f1 = 3.14f;
    double d1 = 2.71828;
    TEST_ASSERT(my_abs(f1 - 3.14f) < EPSILON, "float literal");
    TEST_ASSERT(my_abs(d1 - 2.71828) < DOUBLE_EPSILON, "double literal");

    float sum = f1 + 1.0f;
    TEST_ASSERT(my_abs(sum - 4.14f) < EPSILON, "float +");

    double prod = d1 * 2.0;
    TEST_ASSERT(my_abs(prod - 5.43656) < DOUBLE_EPSILON, "double *");

    /* More arithmetic */
    TEST_ASSERT(my_abs((f1 - 1.0f) - 2.14f) < EPSILON, "float -");
    TEST_ASSERT(my_abs((d1 / 2.0) - 1.35914) < DOUBLE_EPSILON, "double /");

    /* Comparisons */
    TEST_ASSERT(f1 > 3.0f, "float >");
    TEST_ASSERT(d1 < 3.0, "double <");

    /* Type conversions */
    int i = (int)d1;
    TEST_ASSERT(i == 2, "double to int");
    float f_from_int = (float)10;
    TEST_ASSERT(my_abs(f_from_int - 10.0f) < EPSILON, "int to float");

    /* math.h functions */
    float s = sinf(0.0f);
    TEST_ASSERT(my_abs(s) < EPSILON, "sinf(0)");

    double c = cos(M_PI);
    TEST_ASSERT(my_abs(c + 1.0) < DOUBLE_EPSILON, "cos(M_PI)");

    double sqrt2 = sqrt(2.0);
    TEST_ASSERT(my_abs(sqrt2 - 1.414213562) < DOUBLE_EPSILON, "sqrt(2)");

    double p = pow(2.0, 3.0);
    TEST_ASSERT(my_abs(p - 8.0) < DOUBLE_EPSILON, "pow(2,3)");

    double e = exp(1.0);
    TEST_ASSERT(my_abs(e - 2.718281828) < DOUBLE_EPSILON, "exp(1)");

    double l = log(M_E);
    TEST_ASSERT(my_abs(l - 1.0) < DOUBLE_EPSILON, "log(e)");

    float absv = fabsf(-42.0f);
    TEST_ASSERT(my_abs(absv - 42.0f) < EPSILON, "fabsf");

    double fl = floor(3.7);
    TEST_ASSERT(fl == 3.0, "floor");

    double cl = ceil(3.2);
    TEST_ASSERT(cl == 4.0, "ceil");

    double md = fmod(5.5, 2.0);
    TEST_ASSERT(my_abs(md - 1.5) < DOUBLE_EPSILON, "fmod");

    /* Special values */
    float inf = INFINITY;
    TEST_ASSERT(isinf(inf), "INFINITY");
    float nanv = NAN;
    TEST_ASSERT(isnan(nanv), "NAN");

    /* Arrays and pointers */
    float arr[5] = {1.1f, 2.2f, 3.3f, 4.4f, 5.5f};
    float *p_arr = arr;
    TEST_ASSERT(my_abs(p_arr[2] - 3.3f) < EPSILON, "float array/pointer");

    printf("\n=== SUMMARY ===\n");
    printf("Tests passed: %d / %d\n", tests_passed, tests_total);
    if (tests_passed == tests_total) {
        printf("FLOAT + MATH TEST PASSED SUCCESSFULLY!\n");
    } else {
        printf("Some tests failed — check floating-point support in xcc.\n");
    }

    return 0;
}
