/* Each selected conditional arm converts to the expression's common type. */
#define NOINLINE __attribute__((noinline))

static volatile unsigned then_calls, else_calls;
static volatile int narrow_value;
static volatile long wide_value;
static int objects[2] = {17, 29};
typedef char * [[xcc::far]] far_pointer;
static far_pointer chosen_far;

NOINLINE long signed_arm(int condition, int narrow, long wide) {
    return condition ? wide : narrow;
}
NOINLINE long reversed_arm(int condition, int narrow, long wide) {
    return condition ? narrow : wide;
}
NOINLINE unsigned long unsigned_arm(int condition, unsigned narrow,
                                    unsigned long wide) {
    return condition ? wide : narrow;
}
NOINLINE long negative_byte_arm(int condition, signed char narrow, long wide) {
    return condition ? wide : -narrow;
}
NOINLINE unsigned long mixed_sign_arm(int condition, int narrow,
                                     unsigned long wide) {
    return condition ? wide : narrow;
}
#ifdef CONDITIONAL_TEST_WIDE_RUNTIME
NOINLINE long long widest_signed_arm(int condition, long narrow, long long wide) {
    return condition ? narrow : wide;
}
NOINLINE unsigned long long widest_unsigned_arm(int condition, unsigned narrow,
                                               unsigned long long wide) {
    return condition ? wide : narrow;
}
NOINLINE long long widest_byte_arm(int condition, signed char narrow,
                                  long long wide) {
    return condition ? wide : narrow;
}
#endif
NOINLINE long word_to_long_truth(int condition, int narrow, long wide) {
    long result = condition ? wide : narrow;
    return result + 16L ? 9L : 3L;
}
#ifdef CONDITIONAL_TEST_WIDE_RUNTIME
NOINLINE float floating_arm(int condition, int narrow, float wide) {
    return condition ? wide : narrow;
}
#endif
NOINLINE unsigned _BitInt(9) partial_unsigned(int condition,
        unsigned _BitInt(3) narrow, unsigned _BitInt(9) wide) {
    return condition ? wide : narrow;
}
NOINLINE _BitInt(17) partial_signed(int condition,
        _BitInt(9) narrow, _BitInt(17) wide) {
    return condition ? narrow : wide;
}
NOINLINE int *pointer_arm(int condition) {
    return condition ? objects : 0;
}
NOINLINE const int *qualified_pointer_arm(int condition,
                                        int *first, const int *second) {
    return condition ? first : second;
}
struct pair { int first; unsigned second; };
NOINLINE int aggregate_arm(int condition) {
    struct pair first = {3, 5}, second = {7, 11};
    struct pair result = condition ? first : second;
    return result.first + result.second;
}
NOINLINE int narrow_call(void) { ++else_calls; return narrow_value; }
NOINLINE long wide_call(void) { ++then_calls; return wide_value; }
NOINLINE long selected_calls(int condition) {
    return condition ? wide_call() : narrow_call();
}
NOINLINE long selected_objects(int condition) {
    return condition ? wide_value : narrow_value;
}
NOINLINE void selected_far(int condition, char *near_value, far_pointer far_value) {
    chosen_far = condition ? near_value : far_value;
}
NOINLINE void void_arms(int condition) {
    condition ? (void)wide_call() : (void)narrow_call();
}
NOINLINE long constant_arms(void) {
    long first = 0 ? wide_call() : narrow_call();
    long second = 1 ? wide_call() : narrow_call();
    return first + second;
}

static_assert(sizeof(1 ? wide_call() : narrow_call()) == sizeof(long));
static_assert(sizeof(1 ? (char *)0 : (far_pointer)0) == sizeof(far_pointer));
static_assert(sizeof(1 ? (far_pointer)0 : (char *)0) == sizeof(far_pointer));
static_assert(_Generic(1 ? (const int *)0 : (volatile int *)0,
                      const volatile int *: 1, default: 0));
static_assert(_Generic(1 ? (const int *)0 : (volatile void *)0,
                      const volatile void *: 1, default: 0));

int main(void) {
    if (signed_arm(0, -16, 65537L) != -16L) return 1;
    if (signed_arm(1, -16, 65537L) != 65537L) return 2;
    if (signed_arm(0, -32767 - 1, 1L) != -32768L) return 3;
    if (signed_arm(0, 32767, -1L) != 32767L) return 4;
    if (reversed_arm(1, -37, 70000L) != -37L) return 5;
    if (reversed_arm(0, -37, 70000L) != 70000L) return 6;
    if (unsigned_arm(0, 65535U, 70000UL) != 65535UL) return 7;
    if (unsigned_arm(1, 65535U, 70000UL) != 70000UL) return 8;
    if (negative_byte_arm(0, 16, 70000L) != -16L) return 9;
    if (negative_byte_arm(0, -128, 70000L) != 128L) return 10;
    if (mixed_sign_arm(0, -1, 17UL) != 0xffffffffUL) return 11;
    if (word_to_long_truth(0, -16, 7L) != 3L) return 12;
    if (word_to_long_truth(1, -16, 65536L) != 9L) return 13;
#ifdef CONDITIONAL_TEST_WIDE_RUNTIME
    if (floating_arm(0, -13, 3.5f) != -13.0f) return 14;
    if (floating_arm(1, -13, 3.5f) != 3.5f) return 15;
#endif
    if (partial_unsigned(0, 5, 257) != 5) return 16;
    if (partial_unsigned(1, 5, 257) != 257) return 17;
    if ((long)partial_signed(1, -17, 40000) != -17L) return 18;
    if ((long)partial_signed(0, -17, 40000) != 40000L) return 19;
    if (pointer_arm(0) != 0 || pointer_arm(1) != objects) return 20;
    if (qualified_pointer_arm(0, objects, objects + 1) != objects + 1)
        return 21;
    if (qualified_pointer_arm(1, objects, objects + 1) != objects)
        return 22;
    if (aggregate_arm(0) != 18 || aggregate_arm(1) != 8) return 23;
    narrow_value = -16;
    wide_value = 65537L;
    then_calls = else_calls = 0;
    if (selected_calls(0) != -16L || then_calls != 0 || else_calls != 1)
        return 24;
    if (selected_calls(1) != 65537L || then_calls != 1 || else_calls != 1)
        return 25;
    if (selected_objects(0) != -16L || selected_objects(1) != 65537L)
        return 26;
    void_arms(0);
    if (then_calls != 1 || else_calls != 2) return 27;
    void_arms(1);
    if (then_calls != 2 || else_calls != 2) return 28;
    if (constant_arms() != 65521L || then_calls != 3 || else_calls != 3)
        return 29;
    far_pointer far_value = (far_pointer)(char *)objects;
    ((unsigned char *)&far_value)[2] = 19;
    selected_far(0, (char *)objects, far_value);
    if (((unsigned char *)&chosen_far)[2] != 19) return 35;
    if ((char *)chosen_far != (char *)objects) return 36;
    selected_far(1, (char *)objects, far_value);
    if (((unsigned char *)&chosen_far)[2] != 0) return 37;
    if ((char *)chosen_far != (char *)objects) return 38;
#ifdef CONDITIONAL_TEST_WIDE_RUNTIME
    if (widest_signed_arm(1, -70000L, 4294967297LL) != -70000LL)
        return 30;
    if (widest_signed_arm(0, -70000L, 4294967297LL) != 4294967297LL)
        return 31;
    if (widest_unsigned_arm(0, 65535U, 4294967297ULL) != 65535ULL)
        return 32;
    if (widest_unsigned_arm(1, 65535U, 4294967297ULL) != 4294967297ULL)
        return 33;
    if (widest_byte_arm(0, -128, 4294967297LL) != -128LL)
        return 34;
#endif
    return 0;
}
