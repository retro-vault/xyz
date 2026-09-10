/* Casts normalize precision independently of storage size. */
#define NOINLINE __attribute__((noinline))
#define UNSIGNED_CAST(N) \
 NOINLINE unsigned long u##N(unsigned long input) { \
   unsigned _BitInt(N) narrowed = (unsigned _BitInt(N))input; \
   return (unsigned long)narrowed; } \
 NOINLINE unsigned char truth##N(unsigned long input) { \
   return (unsigned _BitInt(N))input ? 1 : 0; }
#define SIGNED_CAST(N) \
 NOINLINE long s##N(unsigned long input) { \
   _BitInt(N) narrowed = (_BitInt(N))input; return (long)narrowed; }
UNSIGNED_CAST(1) UNSIGNED_CAST(3) UNSIGNED_CAST(7)
UNSIGNED_CAST(9) UNSIGNED_CAST(13) UNSIGNED_CAST(15)
UNSIGNED_CAST(17) UNSIGNED_CAST(23) UNSIGNED_CAST(25) UNSIGNED_CAST(31)
SIGNED_CAST(3) SIGNED_CAST(7) SIGNED_CAST(9) SIGNED_CAST(13)
SIGNED_CAST(15) SIGNED_CAST(17) SIGNED_CAST(23) SIGNED_CAST(25) SIGNED_CAST(31)
NOINLINE unsigned word_from_signed(signed char input) {
 return (unsigned _BitInt(9))input;
}
NOINLINE unsigned char byte_truth(signed char input) {
 return (unsigned _BitInt(7))input ? 1 : 0;
}
NOINLINE unsigned char pointer3(void *input) {
 return (unsigned _BitInt(3))input;
}
NOINLINE unsigned long pointer17(void *input) {
 return (unsigned _BitInt(17))input;
}
NOINLINE unsigned char partial_truth(volatile unsigned _BitInt(3) *input) {
 return (unsigned _BitInt(4))*input ? 1 : 0;
}
NOINLINE unsigned long promoted_partial_truth(volatile unsigned _BitInt(3) *input) {
 if ((unsigned _BitInt(4))*input) return 0x13579bdfUL;
 return 0x2468ace0UL;
}
NOINLINE unsigned char copied(unsigned char input) {
 unsigned _BitInt(7) first = (unsigned _BitInt(7))input;
 unsigned _BitInt(3) second = first;
 unsigned _BitInt(5) third = second;
 return third;
}
NOINLINE signed char signed_copy(unsigned char input) {
 unsigned _BitInt(3) first = (unsigned _BitInt(3))input;
 _BitInt(3) second = (_BitInt(3))first;
 return second;
}
NOINLINE unsigned char parameter3(unsigned _BitInt(3) input) { return input; }
NOINLINE unsigned _BitInt(3) return3(unsigned _BitInt(7) input) { return input; }
NOINLINE unsigned char stored(unsigned char input) {
 unsigned _BitInt(7) first = (unsigned _BitInt(7))input;
 struct narrow_pair { unsigned _BitInt(3) a; unsigned _BitInt(5) b; } pair = {first, first};
 unsigned _BitInt(3) array[2] = {0, first};
 pair.a = first;
 array[0] = first;
 if (pair.a != (input & 7) || pair.b != (input & 31) ||
     array[0] != (input & 7) || array[1] != (input & 7)) return 1;
 if (parameter3(first) != (input & 7) || return3(first) != (input & 7)) return 2;
 return 0;
}
static unsigned _BitInt(3) static_u3 = (unsigned _BitInt(3))255;
static _BitInt(13) static_s13 = (_BitInt(13))8191;
static_assert((unsigned _BitInt(3))255 == 7);
static_assert((_BitInt(13))8191 == -1);
static_assert((bool)256);
static const unsigned long inputs[] = {
 0UL, 1UL, 7UL, 8UL, 64UL, 127UL, 128UL, 255UL, 256UL,
 4095UL, 4096UL, 32767UL, 32768UL, 65535UL, 65536UL,
 0x12345678UL, 0x7fffffffUL, 0x80000000UL, 0xffffffffUL
};
#define CHECK_U(N) NOINLINE int check_u##N(void) { \
 unsigned i; for (i = 0; i < sizeof(inputs) / sizeof(inputs[0]); ++i) { \
 unsigned long value = inputs[i]; \
 if (u##N(value) != (value & ((1UL << N) - 1UL))) return N; \
 if (truth##N(value) != ((value & ((1UL << N) - 1UL)) != 0)) return 40 + N; } \
 return 0; }
#define CHECK_S(N) NOINLINE int check_s##N(void) { \
 unsigned i; for (i = 0; i < sizeof(inputs) / sizeof(inputs[0]); ++i) { \
 unsigned long value = inputs[i]; \
 unsigned long low = value & ((1UL << N) - 1UL); \
 long expected = (long)(low ^ (1UL << (N - 1))) - (long)(1UL << (N - 1)); \
 if (s##N(value) != expected) return 80 + N; } return 0; }
CHECK_U(1) CHECK_U(3) CHECK_U(7) CHECK_U(9) CHECK_U(13)
CHECK_U(15) CHECK_U(17) CHECK_U(23) CHECK_U(25) CHECK_U(31)
CHECK_S(3) CHECK_S(7) CHECK_S(9) CHECK_S(13) CHECK_S(15)
CHECK_S(17) CHECK_S(23) CHECK_S(25) CHECK_S(31)
#define RUN_U(N) result = check_u##N(); if (result) return result;
#define RUN_S(N) result = check_s##N(); if (result) return result;
int main(void) {
 unsigned i; int result;
 volatile unsigned _BitInt(3) observed;
 RUN_U(1) RUN_U(3) RUN_U(7) RUN_U(9) RUN_U(13)
 RUN_U(15) RUN_U(17) RUN_U(23) RUN_U(25) RUN_U(31)
 RUN_S(3) RUN_S(7) RUN_S(9) RUN_S(13) RUN_S(15)
 RUN_S(17) RUN_S(23) RUN_S(25) RUN_S(31)
 for (i = 0; i != 256; ++i) {
  unsigned char byte = (unsigned char)i;
  signed char signed_byte = (signed char)byte;
  if (copied(byte) != (i & 7)) return 120;
  if (signed_copy(byte) != (signed char)((i & 7) ^ 4) - 4) return 121;
  if (word_from_signed(signed_byte) != ((unsigned)(int)signed_byte & 511)) return 122;
  if (byte_truth(signed_byte) != ((i & 127) != 0)) return 123;
  if (stored(byte)) return 128;
  observed = (unsigned _BitInt(3))byte;
  if (partial_truth(&observed) != ((i & 7) != 0)) return 132;
  if (promoted_partial_truth(&observed) !=
      ((i & 7) ? 0x13579bdfUL : 0x2468ace0UL)) return 133;
 }
 if ((unsigned _BitInt(3))255 != 7) return 124;
 if ((_BitInt(3))7 != -1) return 125;
 if ((unsigned _BitInt(13))65535 != 8191) return 126;
 if ((_BitInt(13))8191 != -1) return 127;
 if (static_u3 != 7 || static_s13 != -1) return 129;
 if (u3(255) != 7 || s3(7) != -1) return 130;
 if (pointer3((void *)0x8108) || pointer3((void *)0xffff) != 7 ||
     pointer17((void *)0x8108) != 0x8108UL) return 131;
 return 0;
}
