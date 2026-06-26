/*
 * Z80 C23 Advanced Features Test
 * complex, atomic, variadic, attributes
 */

#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __STDC_NO_COMPLEX__
#warning "No complex support"
#else
#include <complex.h>
#endif

#ifdef __STDC_NO_ATOMICS__
#warning "No atomic support"
#else
#include <stdatomic.h>
#endif

#include <stdarg.h>

static int tests_passed = 0;
static int tests_total = 0;

#define TEST_ASSERT(cond, msg) do { \
    tests_total++; \
    if (cond) { tests_passed++; printf("  [PASS] %s\n", msg); } \
    else { printf("  [FAIL] %s\n", msg); } \
} while(0)

/* Variadic example */
int sum_variadic(int count, ...) {
    va_list ap;
    int sum = 0;
    int i;

    va_start(ap, count);
    for (i = 0; i < count; i++) {
        sum += va_arg(ap, int);
    }
    va_end(ap);
    return sum;
}

/* Attribute examples */
[[nodiscard]] static int must_use_result(void) {
    return 42;
}

[[noreturn]] static void never_return(void) {
    while (1) {}
}

int main(void) {
    printf("=== Z80 C23 ADVANCED FEATURES TEST ===\n\n");

    /* Variadic */
    TEST_ASSERT(sum_variadic(4, 10, 20, 30, 40) == 100, "variadic function");

    /* Attributes */
    {
        int res = must_use_result();
        TEST_ASSERT(res == 42, "[[nodiscard]] compiles and works");
    }

    /* Complex (if supported) */
#ifdef complex
    {
        double complex z = 1.0 + 2.0 * I;
        TEST_ASSERT(creal(z) == 1.0 && cimag(z) == 2.0, "complex numbers");
        TEST_ASSERT(cabs(z) > 2.0, "cabs function");
    }
#else
    printf("  [SKIPPED] complex.h not supported\n");
    tests_total++;
#endif

    /* Atomics (if supported) */
#ifdef ATOMIC_INT_LOCK_FREE
    {
        atomic_int atom = 0;
        atomic_store(&atom, 42);
        TEST_ASSERT(atomic_load(&atom) == 42, "atomic_int load/store");
        atomic_fetch_add(&atom, 10);
        TEST_ASSERT(atomic_load(&atom) == 52, "atomic_fetch_add");
    }
#else
    printf("  [SKIPPED] stdatomic.h not supported\n");
    tests_total++;
#endif

    printf("\n=== SUMMARY ===\n");
    printf("Advanced features tests passed: %d / %d\n", tests_passed, tests_total);
    printf("Note: complex and atomic support is optional in C23 and may be missing on Z80.\n");

    return 0;
}
