/* Test enums with explicit underlying bit-precise type.
   This is not an ISO C23 feature, but we'll try to get it into C2Y.
 */
#include <testfwk.h>

#ifdef __SDCC
_Pragma("std_sdcc23")

enum typed_enum_b1 : _BitInt(1) {
  TYPED_ENUM_B1_VAL1 = -1,
  TYPED_ENUM_B1_VAL0 = 0
};

enum typed_enum_bs1 : signed _BitInt(1) {
  TYPED_ENUM_BS1_VAL1 = -1,
  TYPED_ENUM_BS1_VAL0 = 0
};

enum typed_enum_bu1 : unsigned _BitInt(1) {
  TYPED_ENUM_BU1_VAL1 = 1,
  TYPED_ENUM_BU1_VAL0 = 0
};

volatile enum typed_enum_b1 b1;
volatile enum typed_enum_bs1 bs1;
volatile enum typed_enum_bu1 bu1;

#endif

void
testTypedEnum(void)
{
#ifdef __SDCC
  ASSERT(sizeof(enum typed_enum_b1) == sizeof(_BitInt(1)));
  ASSERT(sizeof(enum typed_enum_bs1) == sizeof(signed _BitInt(1)));
  ASSERT(sizeof(enum typed_enum_bu1) == sizeof(unsigned _BitInt(1)));

  b1 = -1;
  ASSERT (b1 == TYPED_ENUM_B1_VAL1);
  bs1 = -1;
  ASSERT (bs1 == TYPED_ENUM_BS1_VAL1);
  bu1 = 1;
  ASSERT (bu1 == TYPED_ENUM_BU1_VAL1);
#endif
}

