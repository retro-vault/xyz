/* auto-generated z88dk ctype shard */
#include "../../upstream/test/suites/ctype/ctype_test.h"

void t_isalnum_0x40()
{
    Assert(isalnum(64)  == 0 ,"isalnum should be 0 for @");
}


void t_isalnum_0x41()
{
    Assert(isalnum(65) ,"isalnum should be 1 for A");
}


void t_isalnum_0x42()
{
    Assert(isalnum(66) ,"isalnum should be 1 for B");
}


void t_isalnum_0x43()
{
    Assert(isalnum(67) ,"isalnum should be 1 for C");
}


void t_isalnum_0x44()
{
    Assert(isalnum(68) ,"isalnum should be 1 for D");
}


void t_isalnum_0x45()
{
    Assert(isalnum(69) ,"isalnum should be 1 for E");
}


void t_isalnum_0x46()
{
    Assert(isalnum(70) ,"isalnum should be 1 for F");
}


void t_isalnum_0x47()
{
    Assert(isalnum(71) ,"isalnum should be 1 for G");
}


void t_isalnum_0x48()
{
    Assert(isalnum(72) ,"isalnum should be 1 for H");
}


void t_isalnum_0x49()
{
    Assert(isalnum(73) ,"isalnum should be 1 for I");
}


void t_isalnum_0x4a()
{
    Assert(isalnum(74) ,"isalnum should be 1 for J");
}


void t_isalnum_0x4b()
{
    Assert(isalnum(75) ,"isalnum should be 1 for K");
}


void t_isalnum_0x4c()
{
    Assert(isalnum(76) ,"isalnum should be 1 for L");
}


void t_isalnum_0x4d()
{
    Assert(isalnum(77) ,"isalnum should be 1 for M");
}


void t_isalnum_0x4e()
{
    Assert(isalnum(78) ,"isalnum should be 1 for N");
}


void t_isalnum_0x4f()
{
    Assert(isalnum(79) ,"isalnum should be 1 for O");
}


void t_isalnum_0x50()
{
    Assert(isalnum(80) ,"isalnum should be 1 for P");
}


void t_isalnum_0x51()
{
    Assert(isalnum(81) ,"isalnum should be 1 for Q");
}


void t_isalnum_0x52()
{
    Assert(isalnum(82) ,"isalnum should be 1 for R");
}


void t_isalnum_0x53()
{
    Assert(isalnum(83) ,"isalnum should be 1 for S");
}


void t_isalnum_0x54()
{
    Assert(isalnum(84) ,"isalnum should be 1 for T");
}


void t_isalnum_0x55()
{
    Assert(isalnum(85) ,"isalnum should be 1 for U");
}


void t_isalnum_0x56()
{
    Assert(isalnum(86) ,"isalnum should be 1 for V");
}


void t_isalnum_0x57()
{
    Assert(isalnum(87) ,"isalnum should be 1 for W");
}


void t_isalnum_0x58()
{
    Assert(isalnum(88) ,"isalnum should be 1 for X");
}


void t_isalnum_0x59()
{
    Assert(isalnum(89) ,"isalnum should be 1 for Y");
}


void t_isalnum_0x5a()
{
    Assert(isalnum(90) ,"isalnum should be 1 for Z");
}


void t_isalnum_0x5b()
{
    Assert(isalnum(91)  == 0 ,"isalnum should be 0 for [");
}


void t_isalnum_0x5c()
{
    Assert(isalnum(92)  == 0 ,"isalnum should be 0 for 0x5c");
}


void t_isalnum_0x5d()
{
    Assert(isalnum(93)  == 0 ,"isalnum should be 0 for ]");
}


void t_isalnum_0x5e()
{
    Assert(isalnum(94)  == 0 ,"isalnum should be 0 for ^");
}


void t_isalnum_0x5f()
{
    Assert(isalnum(95)  == 0 ,"isalnum should be 0 for _");
}


int main(void)
{
    suite_setup("test_isalnum_shard_02");
    suite_add_test(t_isalnum_0x40);
    suite_add_test(t_isalnum_0x41);
    suite_add_test(t_isalnum_0x42);
    suite_add_test(t_isalnum_0x43);
    suite_add_test(t_isalnum_0x44);
    suite_add_test(t_isalnum_0x45);
    suite_add_test(t_isalnum_0x46);
    suite_add_test(t_isalnum_0x47);
    suite_add_test(t_isalnum_0x48);
    suite_add_test(t_isalnum_0x49);
    suite_add_test(t_isalnum_0x4a);
    suite_add_test(t_isalnum_0x4b);
    suite_add_test(t_isalnum_0x4c);
    suite_add_test(t_isalnum_0x4d);
    suite_add_test(t_isalnum_0x4e);
    suite_add_test(t_isalnum_0x4f);
    suite_add_test(t_isalnum_0x50);
    suite_add_test(t_isalnum_0x51);
    suite_add_test(t_isalnum_0x52);
    suite_add_test(t_isalnum_0x53);
    suite_add_test(t_isalnum_0x54);
    suite_add_test(t_isalnum_0x55);
    suite_add_test(t_isalnum_0x56);
    suite_add_test(t_isalnum_0x57);
    suite_add_test(t_isalnum_0x58);
    suite_add_test(t_isalnum_0x59);
    suite_add_test(t_isalnum_0x5a);
    suite_add_test(t_isalnum_0x5b);
    suite_add_test(t_isalnum_0x5c);
    suite_add_test(t_isalnum_0x5d);
    suite_add_test(t_isalnum_0x5e);
    suite_add_test(t_isalnum_0x5f);
    return suite_run();
}
