/* auto-generated z88dk ctype shard */
#include "../../upstream/test/suites/ctype/ctype_test.h"

void t_isalpha_0x40()
{
    Assert(isalpha(64)  == 0 ,"isalpha should be 0 for @");
}


void t_isalpha_0x41()
{
    Assert(isalpha(65) ,"isalpha should be 1 for A");
}


void t_isalpha_0x42()
{
    Assert(isalpha(66) ,"isalpha should be 1 for B");
}


void t_isalpha_0x43()
{
    Assert(isalpha(67) ,"isalpha should be 1 for C");
}


void t_isalpha_0x44()
{
    Assert(isalpha(68) ,"isalpha should be 1 for D");
}


void t_isalpha_0x45()
{
    Assert(isalpha(69) ,"isalpha should be 1 for E");
}


void t_isalpha_0x46()
{
    Assert(isalpha(70) ,"isalpha should be 1 for F");
}


void t_isalpha_0x47()
{
    Assert(isalpha(71) ,"isalpha should be 1 for G");
}


void t_isalpha_0x48()
{
    Assert(isalpha(72) ,"isalpha should be 1 for H");
}


void t_isalpha_0x49()
{
    Assert(isalpha(73) ,"isalpha should be 1 for I");
}


void t_isalpha_0x4a()
{
    Assert(isalpha(74) ,"isalpha should be 1 for J");
}


void t_isalpha_0x4b()
{
    Assert(isalpha(75) ,"isalpha should be 1 for K");
}


void t_isalpha_0x4c()
{
    Assert(isalpha(76) ,"isalpha should be 1 for L");
}


void t_isalpha_0x4d()
{
    Assert(isalpha(77) ,"isalpha should be 1 for M");
}


void t_isalpha_0x4e()
{
    Assert(isalpha(78) ,"isalpha should be 1 for N");
}


void t_isalpha_0x4f()
{
    Assert(isalpha(79) ,"isalpha should be 1 for O");
}


void t_isalpha_0x50()
{
    Assert(isalpha(80) ,"isalpha should be 1 for P");
}


void t_isalpha_0x51()
{
    Assert(isalpha(81) ,"isalpha should be 1 for Q");
}


void t_isalpha_0x52()
{
    Assert(isalpha(82) ,"isalpha should be 1 for R");
}


void t_isalpha_0x53()
{
    Assert(isalpha(83) ,"isalpha should be 1 for S");
}


void t_isalpha_0x54()
{
    Assert(isalpha(84) ,"isalpha should be 1 for T");
}


void t_isalpha_0x55()
{
    Assert(isalpha(85) ,"isalpha should be 1 for U");
}


void t_isalpha_0x56()
{
    Assert(isalpha(86) ,"isalpha should be 1 for V");
}


void t_isalpha_0x57()
{
    Assert(isalpha(87) ,"isalpha should be 1 for W");
}


void t_isalpha_0x58()
{
    Assert(isalpha(88) ,"isalpha should be 1 for X");
}


void t_isalpha_0x59()
{
    Assert(isalpha(89) ,"isalpha should be 1 for Y");
}


void t_isalpha_0x5a()
{
    Assert(isalpha(90) ,"isalpha should be 1 for Z");
}


void t_isalpha_0x5b()
{
    Assert(isalpha(91)  == 0 ,"isalpha should be 0 for [");
}


void t_isalpha_0x5c()
{
    Assert(isalpha(92)  == 0 ,"isalpha should be 0 for 0x5c");
}


void t_isalpha_0x5d()
{
    Assert(isalpha(93)  == 0 ,"isalpha should be 0 for ]");
}


void t_isalpha_0x5e()
{
    Assert(isalpha(94)  == 0 ,"isalpha should be 0 for ^");
}


void t_isalpha_0x5f()
{
    Assert(isalpha(95)  == 0 ,"isalpha should be 0 for _");
}


int main(void)
{
    suite_setup("test_isalpha_shard_02");
    suite_add_test(t_isalpha_0x40);
    suite_add_test(t_isalpha_0x41);
    suite_add_test(t_isalpha_0x42);
    suite_add_test(t_isalpha_0x43);
    suite_add_test(t_isalpha_0x44);
    suite_add_test(t_isalpha_0x45);
    suite_add_test(t_isalpha_0x46);
    suite_add_test(t_isalpha_0x47);
    suite_add_test(t_isalpha_0x48);
    suite_add_test(t_isalpha_0x49);
    suite_add_test(t_isalpha_0x4a);
    suite_add_test(t_isalpha_0x4b);
    suite_add_test(t_isalpha_0x4c);
    suite_add_test(t_isalpha_0x4d);
    suite_add_test(t_isalpha_0x4e);
    suite_add_test(t_isalpha_0x4f);
    suite_add_test(t_isalpha_0x50);
    suite_add_test(t_isalpha_0x51);
    suite_add_test(t_isalpha_0x52);
    suite_add_test(t_isalpha_0x53);
    suite_add_test(t_isalpha_0x54);
    suite_add_test(t_isalpha_0x55);
    suite_add_test(t_isalpha_0x56);
    suite_add_test(t_isalpha_0x57);
    suite_add_test(t_isalpha_0x58);
    suite_add_test(t_isalpha_0x59);
    suite_add_test(t_isalpha_0x5a);
    suite_add_test(t_isalpha_0x5b);
    suite_add_test(t_isalpha_0x5c);
    suite_add_test(t_isalpha_0x5d);
    suite_add_test(t_isalpha_0x5e);
    suite_add_test(t_isalpha_0x5f);
    return suite_run();
}
