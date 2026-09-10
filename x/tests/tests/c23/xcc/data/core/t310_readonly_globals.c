/* Const qualification selects storage for the object, never its pointee. */
const unsigned ro_scalar = 0x1234;
const unsigned ro_zero = 0;
const unsigned ro_implicit;
const unsigned char ro_zero_array[5] = {0};
const char ro_array[] = "named const";
const unsigned ro_nested[2][2] = {{1, 2}, {3, 4}};
typedef unsigned pair[2];
const pair ro_typedef_array = {5, 6};
struct record { unsigned value; char tail[2]; };
const struct record ro_record = {7, {8, 9}};
union number { unsigned word; unsigned char bytes[2]; };
const union number ro_union = {.word = 0x5678};
unsigned rw_word = 17;
unsigned rw_zero;
unsigned * const ro_pointer = &rw_word;
const unsigned * const ro_const_pointer = &ro_scalar;
const char * const ro_pointer_array[2] = {"a", "b"};
const unsigned *rw_pointee_const = &ro_scalar;
const char *rw_pointer_array[2] = {"c", "d"};
const volatile unsigned rw_cv = 19;
const _Atomic unsigned rw_atomic = 23;
const volatile unsigned rw_cv_array[2] = {29, 31};
struct volatile_record { unsigned value; volatile unsigned changing; };
const struct volatile_record rw_volatile_record = {37, 41};
struct atomic_record { _Atomic unsigned changing; };
const struct atomic_record rw_atomic_record = {43};
struct pointer_record { volatile unsigned *address; };
const struct pointer_record ro_volatile_pointee = {&rw_word};
[[xcc::bank(2)]] const unsigned bank_const = 53;
#ifndef READONLY_LINK_ONLY
_Thread_local const unsigned tls_const = 47;
[[sdcc::at(0x9100)]] const unsigned absolute_const;
[[sdcc::sfr(0xfe)]] const unsigned char port_const;

#endif
