#include <float.h>

static_assert(sizeof(float) == 2, "ieee16 float must be 2 bytes");
static_assert(sizeof(double) == 8, "double stays 64-bit");
static_assert(FLT_RADIX == 2, "binary radix");
static_assert(FLT_MANT_DIG == 11, "binary16 mantissa bits");
static_assert(FLT_DIG == 3, "binary16 decimal digits");
static_assert(FLT_MIN_EXP == -13, "binary16 min exponent");
static_assert(FLT_MAX_EXP == 16, "binary16 max exponent");
static_assert(FLT_MIN_10_EXP == -4, "binary16 min decimal exponent");
static_assert(FLT_MAX_10_EXP == 4, "binary16 max decimal exponent");
static_assert(FLT_DECIMAL_DIG == 5, "binary16 round-trip digits");
static_assert(FLT_HAS_SUBNORM == 1, "binary16 has subnormals");
static_assert(FLT_IS_IEC_60559 == 1, "binary16 is IEEE 60559");
static_assert(DBL_MANT_DIG == 53, "double mantissa bits");
static_assert(DBL_DECIMAL_DIG == 17, "double round-trip digits");
static_assert(DBL_IS_IEC_60559 == 1, "double is IEEE 60559");
static_assert(LDBL_MANT_DIG == DBL_MANT_DIG, "long double aliases double");
static_assert(DECIMAL_DIG == 17, "widest runtime type is double");
static_assert(FLT_EVAL_METHOD == 0, "no extra evaluation precision");

int ieee16_header_probe(void) {
    return FLT_MANT_DIG + DBL_MANT_DIG;
}
