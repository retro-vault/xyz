/* Tests usage of constexpr specifier.
 */
#include <testfwk.h>

#if defined(__SDCC) && !defined(__SDCC_pdk14)
_Pragma("std_c23")

// EXAMPLE 1
struct s { void *p; };
constexpr struct s A = { nullptr };
constexpr struct s B = A;

// EXAMPLE 2
constexpr int *p = {}; // Default initialization with a null pointer

// EXAMPLE 3
float f (void) {
    constexpr float f = 1.0f;
    constexpr float g = 3.0f;
    // NOTE: modification of (irrelevant) run-time floating point environment omitted
    constexpr float h = f / g;
    return h;
}


// EXAMPLE 3 NOTE 2
// constexpr unsigned int minusOne = -1; // constraint violation
constexpr unsigned int uint_max = -1U; // ok
// constexpr double onethird = 1.0/3.0; // possible constraint violation
constexpr double onethirdtrunc= (double)(1.0/3.0); // ok
// constexpr _Decimal32 small = DEC64_TRUE_MIN * 0; // constraint violation

// EXAMPLE 3 NOTE 3
typedef unsigned char char8_t;  // borrowed from uchar.h
constexpr char string[] = { "\xFF", }; // ok
constexpr char8_t u8string[] = { u8"\xFF", }; // ok
// constexpr unsigned char ucstring[] = { "\xFF", }; // possible constraint violation



// EXAMPLE 4
constexpr int K = 47;
enum {
    A_ = K, // valid, constant initialization
};
constexpr int L = K; // valid, constexpr initialization
static int b = K + 1; // valid, static initialization
int array[K]; // not a VLA

#endif

void
testConstexpr(void)
{
#if defined(__SDCC) && !defined(__SDCC_pdk14)
    // EXAMPLE 1
    ASSERT(A.p == nullptr);
    // EXAMPLE 2
    ASSERT(p == nullptr);
    // EXAMPLE 3
    ASSERT(f() == (float)(1.0f / 3.0f));
    // EXAMPLE 3 NOTE 2
    ASSERT(uint_max == -1U);
    ASSERT(onethirdtrunc == (double)(1.0/3.0));
    // EXAMPLE 3 NOTE 3
    ASSERT(string[0] == '\xff');
    ASSERT(u8string[0] == (char8_t)0xff);
    // EXAMPLE 4
    ASSERT(K == 47);
    ASSERT(A_ == 47);
    ASSERT(L == 47);
    ASSERT(b == 48);
    ASSERT(sizeof(array) == 47 * sizeof(int));
    // pointer checks
    // ASSERT(&K != &L);
    // shadowing must not interfere
    constexpr int K2 = K;
    {
        constexpr int K = 13;
        constexpr int K3 = K2;
        // K2 (and therefore K3) must not be impacted by the new K
        // and still use the previous definition
        ASSERT(K3 == 47);
    }
#endif
}
