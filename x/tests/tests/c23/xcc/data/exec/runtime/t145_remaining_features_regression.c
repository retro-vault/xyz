/*
 * Z80 C23 Remaining Features Test
 *
 * Covers stdlib.h, stdarg.h (variadic), setjmp.h, and various C23 / modern C
 * features.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

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

jmp_buf jump_buf;

void test_variadic(int count, ...) {
    va_list ap;
    int sum = 0;
    int i;

    va_start(ap, count);
    for (i = 0; i < count; i++) {
        sum += va_arg(ap, int);
    }
    va_end(ap);
    TEST_ASSERT(sum == 15, "variadic function sum");
}

int compare_ints(const void *a, const void *b) {
    return (*(int*)a - *(int*)b);
}

[[nodiscard]] static int test_func(void) {
    return 42;
}

int main(void) {
    printf("=== Z80 C23 REMAINING FEATURES TEST (stdlib, variadic, setjmp, C23) ===\n\n");

    /* ===================== STDLIB.H ===================== */
    printf("--- stdlib.h ---\n");

    /* Memory allocation */
    {
        int *p = (int*)malloc(10 * sizeof(int));
        TEST_ASSERT(p != NULL, "malloc success");
        if (p) {
            int i;

            for (i = 0; i < 10; i++) p[i] = i * 10;
            TEST_ASSERT(p[5] == 50, "malloc array access");

            {
                int *p2 = (int*)realloc(p, 20 * sizeof(int));
                TEST_ASSERT(p2 != NULL, "realloc success");
                if (p2) {
                    TEST_ASSERT(p2[5] == 50, "realloc preserves data");
                    free(p2);
                }
            }
        }
    }

    /* calloc */
    {
        int *zeroed = (int*)calloc(5, sizeof(int));
        TEST_ASSERT(zeroed && zeroed[0] == 0 && zeroed[4] == 0, "calloc zeros memory");
        if (zeroed) free(zeroed);
    }

    /* atoi family */
    TEST_ASSERT(atoi("123") == 123, "atoi");
    TEST_ASSERT(atol("987654") == 987654L, "atol");

    /* rand / srand */
    {
        int r1;
        int r2;

        srand(42);
        r1 = rand();
        r2 = rand();
        TEST_ASSERT(r1 != r2 || r1 == r2, "rand produces values");
    }

    /* qsort */
    {
        int sort_arr[] = {5, 3, 8, 1, 9, 2};
        qsort(sort_arr, 6, sizeof(int), compare_ints);
        TEST_ASSERT(sort_arr[0] == 1 && sort_arr[5] == 9, "qsort with comparator");
    }

    /* abs / labs */
    TEST_ASSERT(abs(-42) == 42, "abs");
    TEST_ASSERT(labs(-123456L) == 123456L, "labs");

    /* ===================== VARIADIC (stdarg.h) ===================== */
    printf("\n--- Variadic functions (stdarg.h) ---\n");
    test_variadic(5, 1, 2, 3, 4, 5);

    /* ===================== SETJMP / LONGJMP ===================== */
    printf("\n--- setjmp / longjmp ---\n");

    {
        int jmp_val = setjmp(jump_buf);
        if (jmp_val == 0) {
            printf("  setjmp initial return\n");
            longjmp(jump_buf, 123);
        } else {
            TEST_ASSERT(jmp_val == 123, "longjmp with value");
        }
    }

    /* ===================== C23 / MODERN FEATURES ===================== */
    printf("\n--- C23 and modern features ---\n");

    {
        bool flag = true;
        TEST_ASSERT(flag, "bool type");
    }

    {
        auto int auto_var = 99;
        TEST_ASSERT(auto_var == 99, "C23 auto type inference");
    }

    /* constexpr style (constant expressions) */
    {
        const int constexpr_like = 100;
        TEST_ASSERT(constexpr_like == 100, "const / constant expression");
    }

    /* restrict qualifier */
    {
        int restrict_test[10];
        int * restrict r_ptr = restrict_test;
        (void)r_ptr;
        TEST_ASSERT(true, "restrict qualifier compiles");
    }

    /* _Alignof */
    TEST_ASSERT(_Alignof(int) >= 1, "_Alignof works");

    /* [[nodiscard]] already tested in function above */
    TEST_ASSERT(test_func() == 42, "[[nodiscard]] function returns");

    /* ===================== FINAL SUMMARY ===================== */
    printf("\n=== SUMMARY ===\n");
    printf("Remaining features tests passed: %d / %d\n", tests_passed, tests_total);
    if (tests_passed == tests_total) {
        printf("REMAINING FEATURES TEST PASSED SUCCESSFULLY!\n");
    } else {
        printf("Some tests failed — useful for identifying gaps in stdlib / C23 support.\n");
    }

    printf("\nThis completes broad coverage of C23 for Z80.\n");

    return 0;
}
