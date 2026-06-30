/* auto-generated z88dk ctype shard */
#include "../../upstream/test/suites/ctype/ctype_test.h"

void t_isupper_0x40()
{
    Assert(isupper(64)  == 0 ,"isupper should be 0 for @");
}


void t_isupper_0x41()
{
    Assert(isupper(65) ,"isupper should be 1 for A");
}


void t_isupper_0x42()
{
    Assert(isupper(66) ,"isupper should be 1 for B");
}


void t_isupper_0x43()
{
    Assert(isupper(67) ,"isupper should be 1 for C");
}


void t_isupper_0x44()
{
    Assert(isupper(68) ,"isupper should be 1 for D");
}


void t_isupper_0x45()
{
    Assert(isupper(69) ,"isupper should be 1 for E");
}


void t_isupper_0x46()
{
    Assert(isupper(70) ,"isupper should be 1 for F");
}


void t_isupper_0x47()
{
    Assert(isupper(71) ,"isupper should be 1 for G");
}


void t_isupper_0x48()
{
    Assert(isupper(72) ,"isupper should be 1 for H");
}


void t_isupper_0x49()
{
    Assert(isupper(73) ,"isupper should be 1 for I");
}


void t_isupper_0x4a()
{
    Assert(isupper(74) ,"isupper should be 1 for J");
}


void t_isupper_0x4b()
{
    Assert(isupper(75) ,"isupper should be 1 for K");
}


void t_isupper_0x4c()
{
    Assert(isupper(76) ,"isupper should be 1 for L");
}


void t_isupper_0x4d()
{
    Assert(isupper(77) ,"isupper should be 1 for M");
}


void t_isupper_0x4e()
{
    Assert(isupper(78) ,"isupper should be 1 for N");
}


void t_isupper_0x4f()
{
    Assert(isupper(79) ,"isupper should be 1 for O");
}


void t_isupper_0x50()
{
    Assert(isupper(80) ,"isupper should be 1 for P");
}


void t_isupper_0x51()
{
    Assert(isupper(81) ,"isupper should be 1 for Q");
}


void t_isupper_0x52()
{
    Assert(isupper(82) ,"isupper should be 1 for R");
}


void t_isupper_0x53()
{
    Assert(isupper(83) ,"isupper should be 1 for S");
}


void t_isupper_0x54()
{
    Assert(isupper(84) ,"isupper should be 1 for T");
}


void t_isupper_0x55()
{
    Assert(isupper(85) ,"isupper should be 1 for U");
}


void t_isupper_0x56()
{
    Assert(isupper(86) ,"isupper should be 1 for V");
}


void t_isupper_0x57()
{
    Assert(isupper(87) ,"isupper should be 1 for W");
}


void t_isupper_0x58()
{
    Assert(isupper(88) ,"isupper should be 1 for X");
}


void t_isupper_0x59()
{
    Assert(isupper(89) ,"isupper should be 1 for Y");
}


void t_isupper_0x5a()
{
    Assert(isupper(90) ,"isupper should be 1 for Z");
}


void t_isupper_0x5b()
{
    Assert(isupper(91)  == 0 ,"isupper should be 0 for [");
}


void t_isupper_0x5c()
{
    Assert(isupper(92)  == 0 ,"isupper should be 0 for 0x5c");
}


void t_isupper_0x5d()
{
    Assert(isupper(93)  == 0 ,"isupper should be 0 for ]");
}


void t_isupper_0x5e()
{
    Assert(isupper(94)  == 0 ,"isupper should be 0 for ^");
}


void t_isupper_0x5f()
{
    Assert(isupper(95)  == 0 ,"isupper should be 0 for _");
}


int main(void)
{
    suite_setup("test_isupper_shard_02");
    suite_add_test(t_isupper_0x40);
    suite_add_test(t_isupper_0x41);
    suite_add_test(t_isupper_0x42);
    suite_add_test(t_isupper_0x43);
    suite_add_test(t_isupper_0x44);
    suite_add_test(t_isupper_0x45);
    suite_add_test(t_isupper_0x46);
    suite_add_test(t_isupper_0x47);
    suite_add_test(t_isupper_0x48);
    suite_add_test(t_isupper_0x49);
    suite_add_test(t_isupper_0x4a);
    suite_add_test(t_isupper_0x4b);
    suite_add_test(t_isupper_0x4c);
    suite_add_test(t_isupper_0x4d);
    suite_add_test(t_isupper_0x4e);
    suite_add_test(t_isupper_0x4f);
    suite_add_test(t_isupper_0x50);
    suite_add_test(t_isupper_0x51);
    suite_add_test(t_isupper_0x52);
    suite_add_test(t_isupper_0x53);
    suite_add_test(t_isupper_0x54);
    suite_add_test(t_isupper_0x55);
    suite_add_test(t_isupper_0x56);
    suite_add_test(t_isupper_0x57);
    suite_add_test(t_isupper_0x58);
    suite_add_test(t_isupper_0x59);
    suite_add_test(t_isupper_0x5a);
    suite_add_test(t_isupper_0x5b);
    suite_add_test(t_isupper_0x5c);
    suite_add_test(t_isupper_0x5d);
    suite_add_test(t_isupper_0x5e);
    suite_add_test(t_isupper_0x5f);
    return suite_run();
}
