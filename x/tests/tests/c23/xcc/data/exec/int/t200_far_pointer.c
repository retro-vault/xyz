//
// Far (24-bit banked) pointer execution test.
//
// On the default/unbanked target the far-access trampoline is identity
// (bank ignored), so a far pointer to an ordinary RAM object must behave
// exactly like a near pointer.  This exercises declaration, load, store,
// indexing, pointer arithmetic, and near<->far casts.
//
#include "xcc_exec_test.h"

typedef char * [[xcc::far]] fcharp;
typedef int  * [[xcc::far]] fintp;
union far_bytes {
    fcharp pointer;
    unsigned char bytes[3];
};

static char buf[8] = { 10, 20, 30, 40, 50, 60, 70, 80 };
static int  iarr[4] = { 1000, 2000, 3000, 4000 };

int main(void) {
    // sizeof: far pointer is 3 bytes, near is 2.
    XCC_CHECK_EQ_INT_ID(1, (int)sizeof(fcharp), 3);
    XCC_CHECK_EQ_INT_ID(2, (int)sizeof(char *), 2);

    // Far load through a far pointer (byte).
    fcharp fp = (fcharp)&buf[0];
    XCC_CHECK_EQ_INT_ID(3, *fp, 10);

    // Far indexing (byte) keeps the bank and adjusts the 16-bit address.
    XCC_CHECK_EQ_INT_ID(4, fp[3], 40);
    XCC_CHECK_EQ_INT_ID(5, fp[7], 80);

    // Far pointer arithmetic and dereference.
    fcharp q = fp + 5;
    XCC_CHECK_EQ_INT_ID(6, *q, 60);
    q = q - 2;
    XCC_CHECK_EQ_INT_ID(7, *q, 40);

    // Far store through a far pointer (byte).
    fp[2] = 99;
    XCC_CHECK_EQ_INT_ID(8, buf[2], 99);
    *fp = 11;
    XCC_CHECK_EQ_INT_ID(9, buf[0], 11);

    // Far load/store of a 2-byte object (int).
    fintp ip = (fintp)&iarr[0];
    XCC_CHECK_EQ_INT_ID(10, *ip, 1000);
    XCC_CHECK_EQ_INT_ID(11, ip[2], 3000);
    ip[1] = 1234;
    XCC_CHECK_EQ_INT_ID(12, iarr[1], 1234);
    *(ip + 3) = 4321;
    XCC_CHECK_EQ_INT_ID(13, iarr[3], 4321);

    // near -> far -> near round-trip preserves the address.
    char *np = &buf[4];
    fcharp ff = (fcharp)np;
    char *back = (char *)ff;
    XCC_CHECK_EQ_INT_ID(14, *back, 50);
    XCC_CHECK_ID(15, back == np);

    // Packed ABI: bank first, then little-endian address.
    union far_bytes layout = { .pointer = (fcharp)&buf[0] };
    unsigned near_address = (unsigned)(char *)&buf[0];
    XCC_CHECK_EQ_INT_ID(16, layout.bytes[0], 0);
    XCC_CHECK_EQ_INT_ID(17, layout.bytes[1], near_address & 0xff);
    XCC_CHECK_EQ_INT_ID(18, layout.bytes[2], near_address >> 8);

    // Address arithmetic never carries into the logical bank byte.
    layout.bytes[0] = 7;
    union far_bytes advanced = { .pointer = layout.pointer + 3 };
    XCC_CHECK_EQ_INT_ID(19, advanced.bytes[0], 7);
    XCC_CHECK_EQ_INT_ID(20, *advanced.pointer, 40);

    // Truth/comparison use the address lane correctly when its low byte is 0.
    union far_bytes address_0100 = { .bytes = { 0, 0, 1 } };
    XCC_CHECK_ID(21, address_0100.pointer != (fcharp)0);
    XCC_CHECK_ID(22, !!address_0100.pointer);

    // Equality includes the bank even when the address lane is zero.
    union far_bytes bank_only = { .bytes = { 7, 0, 0 } };
    XCC_CHECK_ID(23, bank_only.pointer != (fcharp)0);
    XCC_CHECK_ID(24, bank_only.pointer != address_0100.pointer);

    return 0;
}
