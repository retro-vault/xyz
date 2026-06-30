/* auto-generated z88dk ctype shard */
#include "../../upstream/test/suites/ctype/ctype_test.h"

void t_isascii_0x40()
{
    Assert(isascii(64) ,"isascii should be 1 for @");
}


void t_isascii_0x41()
{
    Assert(isascii(65) ,"isascii should be 1 for A");
}


void t_isascii_0x42()
{
    Assert(isascii(66) ,"isascii should be 1 for B");
}


void t_isascii_0x43()
{
    Assert(isascii(67) ,"isascii should be 1 for C");
}


void t_isascii_0x44()
{
    Assert(isascii(68) ,"isascii should be 1 for D");
}


void t_isascii_0x45()
{
    Assert(isascii(69) ,"isascii should be 1 for E");
}


void t_isascii_0x46()
{
    Assert(isascii(70) ,"isascii should be 1 for F");
}


void t_isascii_0x47()
{
    Assert(isascii(71) ,"isascii should be 1 for G");
}


void t_isascii_0x48()
{
    Assert(isascii(72) ,"isascii should be 1 for H");
}


void t_isascii_0x49()
{
    Assert(isascii(73) ,"isascii should be 1 for I");
}


void t_isascii_0x4a()
{
    Assert(isascii(74) ,"isascii should be 1 for J");
}


void t_isascii_0x4b()
{
    Assert(isascii(75) ,"isascii should be 1 for K");
}


void t_isascii_0x4c()
{
    Assert(isascii(76) ,"isascii should be 1 for L");
}


void t_isascii_0x4d()
{
    Assert(isascii(77) ,"isascii should be 1 for M");
}


void t_isascii_0x4e()
{
    Assert(isascii(78) ,"isascii should be 1 for N");
}


void t_isascii_0x4f()
{
    Assert(isascii(79) ,"isascii should be 1 for O");
}


void t_isascii_0x50()
{
    Assert(isascii(80) ,"isascii should be 1 for P");
}


void t_isascii_0x51()
{
    Assert(isascii(81) ,"isascii should be 1 for Q");
}


void t_isascii_0x52()
{
    Assert(isascii(82) ,"isascii should be 1 for R");
}


void t_isascii_0x53()
{
    Assert(isascii(83) ,"isascii should be 1 for S");
}


void t_isascii_0x54()
{
    Assert(isascii(84) ,"isascii should be 1 for T");
}


void t_isascii_0x55()
{
    Assert(isascii(85) ,"isascii should be 1 for U");
}


void t_isascii_0x56()
{
    Assert(isascii(86) ,"isascii should be 1 for V");
}


void t_isascii_0x57()
{
    Assert(isascii(87) ,"isascii should be 1 for W");
}


void t_isascii_0x58()
{
    Assert(isascii(88) ,"isascii should be 1 for X");
}


void t_isascii_0x59()
{
    Assert(isascii(89) ,"isascii should be 1 for Y");
}


void t_isascii_0x5a()
{
    Assert(isascii(90) ,"isascii should be 1 for Z");
}


void t_isascii_0x5b()
{
    Assert(isascii(91) ,"isascii should be 1 for [");
}


void t_isascii_0x5c()
{
    Assert(isascii(92) ,"isascii should be 1 for 0x5c");
}


void t_isascii_0x5d()
{
    Assert(isascii(93) ,"isascii should be 1 for ]");
}


void t_isascii_0x5e()
{
    Assert(isascii(94) ,"isascii should be 1 for ^");
}


void t_isascii_0x5f()
{
    Assert(isascii(95) ,"isascii should be 1 for _");
}


int main(void)
{
    suite_setup("test_isascii_shard_02");
    suite_add_test(t_isascii_0x40);
    suite_add_test(t_isascii_0x41);
    suite_add_test(t_isascii_0x42);
    suite_add_test(t_isascii_0x43);
    suite_add_test(t_isascii_0x44);
    suite_add_test(t_isascii_0x45);
    suite_add_test(t_isascii_0x46);
    suite_add_test(t_isascii_0x47);
    suite_add_test(t_isascii_0x48);
    suite_add_test(t_isascii_0x49);
    suite_add_test(t_isascii_0x4a);
    suite_add_test(t_isascii_0x4b);
    suite_add_test(t_isascii_0x4c);
    suite_add_test(t_isascii_0x4d);
    suite_add_test(t_isascii_0x4e);
    suite_add_test(t_isascii_0x4f);
    suite_add_test(t_isascii_0x50);
    suite_add_test(t_isascii_0x51);
    suite_add_test(t_isascii_0x52);
    suite_add_test(t_isascii_0x53);
    suite_add_test(t_isascii_0x54);
    suite_add_test(t_isascii_0x55);
    suite_add_test(t_isascii_0x56);
    suite_add_test(t_isascii_0x57);
    suite_add_test(t_isascii_0x58);
    suite_add_test(t_isascii_0x59);
    suite_add_test(t_isascii_0x5a);
    suite_add_test(t_isascii_0x5b);
    suite_add_test(t_isascii_0x5c);
    suite_add_test(t_isascii_0x5d);
    suite_add_test(t_isascii_0x5e);
    suite_add_test(t_isascii_0x5f);
    return suite_run();
}
