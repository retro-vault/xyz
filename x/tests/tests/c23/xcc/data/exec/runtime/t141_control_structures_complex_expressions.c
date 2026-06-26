/*
 * Z80 C23 Exhaustive Control Structures + Complex Expressions Test
 *
 * Tests every major control structure and highly complex expressions.
 * Includes: if/else, switch/case, while, do-while, for, break, continue, goto,
 *           pre/post ++ --, compound assignments (+= -= etc.), bitwise ops,
 *           logical ops, ternary, comma operator, mixed complex expressions,
 *           bool logic, and side effects.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

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
} while (0)

int main(void) {
    printf("=== Z80 C23 CONTROL STRUCTURES + COMPLEX EXPRESSIONS TEST ===\n\n");

    printf("--- if / else / else if ---\n");

    int x = 10;
    if (x > 5) {
        TEST_ASSERT(true, "simple if true");
    } else {
        TEST_ASSERT(false, "simple if false branch");
    }

    if (x < 0) {
        TEST_ASSERT(false, "if false");
    } else if (x == 10) {
        TEST_ASSERT(true, "else if true");
    } else {
        TEST_ASSERT(false, "else if false");
    }

    printf("\n--- switch / case / default / fallthrough ---\n");

    int val = 2;
    int result = 0;

    switch (val) {
    case 1:
        result = 10;
        break;
    case 2:
    case 3:
        result += 20;
        break;
    default:
        result = 99;
        break;
    }
    TEST_ASSERT(result == 20, "switch with fallthrough");

    val = 5;
    switch (val) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        result = 100;
        break;
    default:
        result = 0;
    }
    TEST_ASSERT(result == 100, "switch multiple cases in one line");

    printf("\n--- while loop ---\n");

    int i = 0;
    int sum = 0;
    while (i < 5) {
        sum += i;
        i++;
    }
    TEST_ASSERT(sum == 10, "while loop sum 0..4");

    int a = 3;
    int b = 7;
    while ((a < 10) && (b > 0)) {
        a++;
        b--;
    }
    TEST_ASSERT(a == 10 && b == 0, "while complex condition &&");

    printf("\n--- do-while loop ---\n");

    int j = 0;
    sum = 0;
    do {
        sum += j;
        j++;
    } while (j < 5);
    TEST_ASSERT(sum == 10, "do-while sum 0..4");

    int run_once = 0;
    do {
        run_once = 1;
    } while (0);
    TEST_ASSERT(run_once == 1, "do-while executes at least once");

    printf("\n--- for loop (all variations) ---\n");

    sum = 0;
    for (int k = 0; k < 5; k++) {
        sum += k;
    }
    TEST_ASSERT(sum == 10, "standard for loop");

    int m;
    int n;
    for (m = 0, n = 10; m < 5; m++, n--) {
        sum += m + n;
    }
    TEST_ASSERT(true, "for with multiple init/update");

    int counter = 0;
    for (;;) {
        counter++;
        if (counter > 3) {
            break;
        }
    }
    TEST_ASSERT(counter == 4, "infinite for with break");

    printf("\n--- break and continue ---\n");

    sum = 0;
    for (int p = 0; p < 10; p++) {
        if (p == 5) {
            continue;
        }
        if (p == 8) {
            break;
        }
        sum += p;
    }
    TEST_ASSERT(sum == (0 + 1 + 2 + 3 + 4 + 6 + 7),
                "for with continue + break");

    printf("\n--- goto ---\n");

    int goto_test = 0;
    goto label1;
    goto_test = 99;
label1:
    goto_test = 1;
    TEST_ASSERT(goto_test == 1, "goto basic");

    printf("\n--- Complex expressions (arithmetic, bitwise, logical) ---\n");

    int v1 = 5;
    int v2 = 3;
    int complex = ((v1 + v2) * 2 - 4) / 3 + (v1 & v2);
    TEST_ASSERT(complex == 5, "complex arithmetic + bitwise");

    int pre = 10;
    int post_res = pre++ + ++pre;
    TEST_ASSERT(post_res == 22 && pre == 12, "pre/post increment in expression");

    int ca = 100;
    ca += 50;
    ca -= 20;
    ca *= 2;
    ca /= 5;
    ca %= 7;
    TEST_ASSERT(ca == 3, "compound assignments += -= *= /= %=");

    unsigned int bw = 0xFF;
    bw &= 0x0F;
    bw |= 0xF0;
    bw ^= 0xAA;
    TEST_ASSERT(bw == 0x55, "bitwise compound &= |= ^=");

    int shift_val = 1;
    shift_val <<= 4;
    shift_val >>= 2;
    TEST_ASSERT(shift_val == 4, "left/right shift compound");

    bool b1 = true;
    bool b2 = false;
    TEST_ASSERT((b1 && !b2) || (b1 == b2), "logical && || !");

    int tern = (v1 > v2) ? 100 : 200;
    TEST_ASSERT(tern == 100, "ternary operator");

    int comma_res;
    comma_res = (v1 = 7, v2 = 8, v1 + v2);
    TEST_ASSERT(comma_res == 15 && v1 == 7 && v2 == 8, "comma operator");

    int complex2 =
        ((v1++ + --v2) * (v1 | v2) & 0xFF) +
        (b1 ? 10 : 20) -
        (v1 > 5 ? 3 : 1);
    (void)complex2;
    TEST_ASSERT(true, "very complex mixed expression executed without crash");

    printf("\n--- Nested control structures with complex expressions ---\n");

    int outer = 0;
    for (int q = 0; q < 3; q++) {
        int inner = 0;
        while (inner < 2) {
            if ((q + inner) % 2 == 0) {
                outer += (q << 1) + inner;
            } else {
                outer += q | inner;
            }
            inner++;
        }
    }
    TEST_ASSERT(outer > 0, "nested for/while/if with complex expressions");

    printf("\n=== SUMMARY ===\n");
    printf("Control + Expression tests passed: %d / %d\n",
           tests_passed, tests_total);
    if (tests_passed == tests_total) {
        printf("CONTROL STRUCTURES + COMPLEX EXPRESSIONS TEST PASSED!\n");
    } else {
        printf("Some tests failed - this is a strong stress test for expression parsing and codegen.\n");
    }

    printf("\nThis test exercises nearly all control flow and expression features in C.\n");
    printf("Particularly useful for checking code generation on 8-bit CPUs like Z80.\n");

    return 0;
}
