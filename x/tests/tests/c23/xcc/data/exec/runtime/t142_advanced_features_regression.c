/*
 * Z80 C23 Advanced Features Test
 * complex, atomic, variadic, attributes, generic selection,
 * compound literals, flexible arrays, anonymous unions, restrict
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
static int tests_skipped = 0;
static int failed_count = 0;

#define MAX_FAILURES 256

static const char *failed_tests[MAX_FAILURES];

#define TEST_ASSERT(cond, msg) do { \
    tests_total++; \
    if (cond) { \
        tests_passed++; \
        printf("  [PASS] %s\n", msg); \
    } else { \
        printf("  [FAIL] %s\n", msg); \
        if (failed_count < MAX_FAILURES) { \
            failed_tests[failed_count++] = msg; \
        } \
    } \
} while(0)

#define TEST_SKIP(msg) do { \
    tests_skipped++; \
    printf("  [SKIPPED] %s\n", msg); \
} while(0)

struct FlexPacket {
    unsigned char length;
    unsigned char payload[];
};

struct AnonymousBox {
    int tag;
    union {
        int value;
        unsigned char bytes[sizeof(int)];
    };
};

_Static_assert(sizeof(char) == 1, "char must be one byte");
_Static_assert(sizeof(struct FlexPacket) == sizeof(unsigned char),
               "flexible array members do not add to sizeof");

#define TYPE_TAG(expr) _Generic((expr), \
    int: 1, \
    long: 2, \
    double: 3, \
    const char *: 4, \
    struct AnonymousBox: 5, \
    default: 0)

/* Variadic example */
static int sum_variadic(int count, ...) {
    va_list ap;
    int i;
    int sum;

    va_start(ap, count);
    sum = 0;
    for (i = 0; i < count; i++) {
        sum += va_arg(ap, int);
    }
    va_end(ap);
    return sum;
}

static int sum_variadic_copy(int count, ...) {
    va_list ap;
    va_list copy;
    int i;
    int first_sum;
    int second_sum;

    va_start(ap, count);
    va_copy(copy, ap);

    first_sum = 0;
    second_sum = 0;
    for (i = 0; i < count; i++) {
        first_sum += va_arg(ap, int);
        second_sum += va_arg(copy, int);
    }

    va_end(copy);
    va_end(ap);
    if (first_sum != second_sum) {
        return -1;
    }
    return first_sum;
}

/* Attribute examples */
[[nodiscard]] static int must_use_result(void) {
    return 42;
}

[[noreturn]] static void never_return(void) {
    while (1) {}
}

[[maybe_unused]] static int attribute_demo_value = 7;

static int sum_restrict_pair(const int *restrict left, const int *restrict right) {
    return left[0] + right[0];
}

int main(void) {
    int failed_total;

    printf("=== Z80 C23 ADVANCED FEATURES TEST ===\n\n");

    /* Variadic */
    printf("--- Variadic functions ---\n");
    TEST_ASSERT(sum_variadic(4, 10, 20, 30, 40) == 100, "variadic function");
    TEST_ASSERT(sum_variadic(0) == 0, "variadic zero arguments");
    TEST_ASSERT(sum_variadic_copy(3, 7, 8, 9) == 24, "va_copy duplicates argument walk");

    /* Attributes */
    printf("\n--- Standard attributes ---\n");
    int res = must_use_result();
    int attr_switch;

    TEST_ASSERT(res == 42, "[[nodiscard]] compiles and works");
    attr_switch = 0;
    switch (1) {
        case 1:
            attr_switch += 2;
            [[fallthrough]];
        case 2:
            attr_switch += 3;
            break;
        default:
            attr_switch = -1;
            break;
    }
    TEST_ASSERT(attr_switch == 5, "[[fallthrough]] attribute");

    /* Generic selection */
    printf("\n--- Generic selection ---\n");
    TEST_ASSERT(TYPE_TAG(123) == 1, "_Generic selects int");
    TEST_ASSERT(TYPE_TAG(123L) == 2, "_Generic selects long");
    TEST_ASSERT(TYPE_TAG(3.0) == 3, "_Generic selects double");
    TEST_ASSERT(TYPE_TAG((const char *)"z80") == 4, "_Generic selects pointer");

    /* Compound literals and layout features */
    printf("\n--- Compound literals and layout ---\n");
    {
        const struct AnonymousBox *literal_box;
        struct AnonymousBox box;
        int compound_sum;
        unsigned char packet_storage[sizeof(struct FlexPacket) + 4];
        struct FlexPacket *packet;
        int left_values[1];
        int right_values[1];

        literal_box = &(struct AnonymousBox){ .tag = 1, .value = 0x1234 };
        TEST_ASSERT(literal_box->tag == 1 && literal_box->value == 0x1234,
                    "compound literal struct");

        box = (struct AnonymousBox){ .tag = 2, .value = 0x1234 };
        TEST_ASSERT(box.bytes[0] == 0x34, "anonymous union byte access");
        TEST_ASSERT(TYPE_TAG(box) == 5, "_Generic selects struct type");

        compound_sum = ((int[]){2, 4, 6})[1] + ((int[]){1, 3, 5})[2];
        TEST_ASSERT(compound_sum == 9, "compound literal array");

        packet = (struct FlexPacket *)packet_storage;
        packet->length = 4;
        packet->payload[0] = 10;
        packet->payload[1] = 20;
        packet->payload[2] = 30;
        packet->payload[3] = 40;
        TEST_ASSERT(packet->length == 4 && packet->payload[3] == 40,
                    "flexible array member");

        left_values[0] = 12;
        right_values[0] = 30;
        TEST_ASSERT(sum_restrict_pair(left_values, right_values) == 42,
                    "restrict-qualified pointers");
    }

    /* Complex (if supported) */
    printf("\n--- Complex numbers ---\n");
#if !defined(__STDC_NO_COMPLEX__)
    double complex z = 1.0 + 2.0 * I;
    double complex w = 2.0 - 1.0 * I;
    double complex product = z * w;

    TEST_ASSERT(creal(z) == 1.0 && cimag(z) == 2.0, "complex numbers");
    TEST_ASSERT(creal(product) == 4.0 && cimag(product) == 3.0, "complex multiply");
    TEST_ASSERT(cimag(conj(z)) == -2.0, "complex conjugate");
    TEST_ASSERT(cabs(z) > 2.0, "cabs function");
#else
    TEST_SKIP("complex.h not supported");
#endif

    /* Atomics (if supported) */
    printf("\n--- Atomics ---\n");
#if !defined(__STDC_NO_ATOMICS__)
    atomic_int atom = 0;
    int previous;
    int expected;
    atomic_flag flag = ATOMIC_FLAG_INIT;

    atomic_store(&atom, 42);
    TEST_ASSERT(atomic_load(&atom) == 42, "atomic_int load/store");
    atomic_fetch_add(&atom, 10);
    TEST_ASSERT(atomic_load(&atom) == 52, "atomic_fetch_add");
    previous = atomic_exchange(&atom, 7);
    TEST_ASSERT(previous == 52 && atomic_load(&atom) == 7, "atomic_exchange");
    expected = 7;
    TEST_ASSERT(atomic_compare_exchange_strong(&atom, &expected, 99) &&
                atomic_load(&atom) == 99, "atomic_compare_exchange_strong");
    TEST_ASSERT(!atomic_flag_test_and_set(&flag), "atomic_flag first set");
    TEST_ASSERT(atomic_flag_test_and_set(&flag), "atomic_flag second set");
    atomic_flag_clear(&flag);
#else
    TEST_SKIP("stdatomic.h not supported");
#endif

    printf("\n=== SUMMARY ===\n");
    printf("Advanced features tests passed: %d / %d\n", tests_passed, tests_total);
    failed_total = tests_total - tests_passed;
    printf("Failed: %d  Skipped: %d\n", failed_total, tests_skipped);
    if (failed_total == 0) {
        printf("ADVANCED FEATURES TEST PASSED!\n");
    } else {
        int failure_index;

        printf("Failed tests at end:\n");
        for (failure_index = 0; failure_index < failed_count; failure_index++) {
            printf("  [FAIL] %s\n", failed_tests[failure_index]);
        }
    }

    printf("Note: complex and atomic support is optional in C23 and may be missing on Z80.\n");
    printf("This file now covers more than the original set of advanced features.\n");

    return 0;
}
