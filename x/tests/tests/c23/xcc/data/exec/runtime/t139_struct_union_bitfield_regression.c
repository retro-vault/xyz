/*
 * Z80 C23 Struct / Union / Bitfield / Typedef Regression Test
 *
 * Derived from the broader feature test shared during debugging.
 * xcc targets 16-bit int, so the union overlap check uses int32_t for
 * the 32-bit fixed-width member instead of plain int.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

static int tests_passed = 0;
static int tests_total = 0;
static int failed_count = 0;

#define MAX_FAILURES 256

static const char *failed_tests[MAX_FAILURES];

#define TEST_ASSERT(cond, msg) do { \
    tests_total++; \
    if (cond) { tests_passed++; printf("  [PASS] %s\n", msg); } \
    else { \
        printf("  [FAIL] %s\n", msg); \
        if (failed_count < MAX_FAILURES) { \
            failed_tests[failed_count++] = msg; \
        } \
    } \
} while(0)

int main(void) {
    printf("=== Z80 C23 STRUCT / UNION / BITFIELD / TYPEDEF TEST ===\n\n");

    printf("--- Basic structs ---\n");

    struct Point {
        int x;
        int y;
    };
    struct Point p1 = {10, 20};
    TEST_ASSERT(p1.x == 10 && p1.y == 20, "basic struct init");

    struct Point p2;
    p2.x = 5;
    p2.y = 15;
    TEST_ASSERT(p2.x + p2.y == 20, "struct member assignment");

    struct Point *pp = &p1;
    TEST_ASSERT(pp->x == 10, "struct pointer -> access");

    printf("\n--- Nested structs ---\n");

    struct Rect {
        struct Point top_left;
        struct Point bottom_right;
    };
    struct Rect r = {{0, 0}, {100, 100}};
    TEST_ASSERT(r.top_left.x == 0 && r.bottom_right.y == 100, "nested struct");

    printf("\n--- Anonymous structs ---\n");

    struct Container {
        int id;
        struct {
            int width;
            int height;
        };
    };
    struct Container c = {42, {800, 600}};
    TEST_ASSERT(c.id == 42 && c.width == 800, "anonymous struct member access");

    printf("\n--- Unions ---\n");

    union Data {
        int32_t i;
        float f;
        uint8_t bytes[4];
    };
    union Data u;
    u.i = 0x12345678L;
    TEST_ASSERT(u.bytes[0] == 0x78, "union type punning (little-endian Z80)");
    TEST_ASSERT(u.i == 0x12345678L, "union int32 access");

    u.f = 3.14f;
    TEST_ASSERT(u.i != 0x12345678L, "union float overwrite");

    TEST_ASSERT(sizeof(union Data) >= 4, "union size at least largest member");

    printf("\n--- Bitfields ---\n");

    struct Flags {
        unsigned int a : 3;
        unsigned int b : 5;
        int          c : 4;
        unsigned int d : 1;
        unsigned int   : 3;
        unsigned int e : 2;
    };

    struct Flags f = {0};
    f.a = 5;
    f.b = 17;
    f.c = -3;
    f.d = 1;
    f.e = 2;

    TEST_ASSERT(f.a == 5, "bitfield unsigned 3-bit");
    TEST_ASSERT(f.b == 17, "bitfield unsigned 5-bit");
    TEST_ASSERT(f.c == -3, "bitfield signed 4-bit");
    TEST_ASSERT(f.d == 1, "bitfield 1-bit");
    TEST_ASSERT(f.e == 2, "bitfield after padding");

    printf("  sizeof(struct Flags) = %zu bytes (expected small due to packing)\n",
           sizeof(struct Flags));

    struct MixedBit {
        uint8_t header;
        unsigned int flags : 4;
        uint16_t data;
    };
    struct MixedBit mb = {0xAA, 0xF, 0x1234};
    TEST_ASSERT(mb.header == 0xAA && mb.flags == 0xF && mb.data == 0x1234, "mixed bitfield + normal members");

    printf("\n--- Typedefs ---\n");

    typedef struct Point Point_t;
    typedef union Data Data_t;
    typedef struct Flags Flags_t;

    Point_t pt = {1, 2};
    Data_t dt;
    dt.i = 999;
    Flags_t fl;
    fl.a = 3;

    TEST_ASSERT(pt.x == 1 && dt.i == 999 && fl.a == 3, "typedef usage");

    typedef int IntArray[10];
    IntArray arr;
    arr[5] = 42;
    TEST_ASSERT(arr[5] == 42, "typedef array");

    printf("\n--- Designated initializers ---\n");

    struct Point p3 = {.y = 99, .x = 11};
    TEST_ASSERT(p3.x == 11 && p3.y == 99, "designated initializer");

    struct Flags f2 = {.b = 31, .a = 7, .c = 5};
    TEST_ASSERT(f2.a == 7 && f2.b == 31 && f2.c == 5, "designated bitfield init");

    printf("\n--- offsetof / sizeof ---\n");

    TEST_ASSERT(offsetof(struct Point, y) == sizeof(int), "offsetof basic");
    TEST_ASSERT(sizeof(struct Rect) >= 2 * sizeof(struct Point), "sizeof nested struct");

    printf("\n--- Arrays of structs ---\n");

    struct Point points[3] = {{1, 2}, {3, 4}, {5, 6}};
    TEST_ASSERT(points[1].x == 3, "array of structs");

    printf("\n--- Pointers to structs/unions ---\n");

    struct Point *p_ptr = &p1;
    p_ptr->x = 123;
    TEST_ASSERT(p1.x == 123, "arrow operator assignment");

    printf("\n=== SUMMARY ===\n");
    printf("Struct/Union/Bitfield tests passed: %d / %d\n", tests_passed, tests_total);
    if (tests_passed == tests_total) {
        printf("STRUCT / UNION / BITFIELD TEST PASSED!\n");
    } else {
        printf("Some tests failed - check struct layout, bitfield packing, or alignment in your compiler.\n");
        printf("Failed tests at end:\n");
        for (int failure_index = 0; failure_index < failed_count; failure_index++) {
            printf("  [FAIL] %s\n", failed_tests[failure_index]);
        }
    }

    printf("\nNote: Bitfield packing and padding are implementation-defined.\n");
    printf("Z80 is little-endian; bitfields typically start from LSB.\n");

    return 0;
}
