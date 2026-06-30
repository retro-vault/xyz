/* auto-generated z88dk ctype shard */
#include "../../upstream/test/suites/ctype/ctype_test.h"

void t_ispunct_0x40()
{
    Assert(ispunct(64) ,"ispunct should be 1 for @");
}


void t_ispunct_0x41()
{
    Assert(ispunct(65)  == 0 ,"ispunct should be 0 for A");
}


void t_ispunct_0x42()
{
    Assert(ispunct(66)  == 0 ,"ispunct should be 0 for B");
}


void t_ispunct_0x43()
{
    Assert(ispunct(67)  == 0 ,"ispunct should be 0 for C");
}


void t_ispunct_0x44()
{
    Assert(ispunct(68)  == 0 ,"ispunct should be 0 for D");
}


void t_ispunct_0x45()
{
    Assert(ispunct(69)  == 0 ,"ispunct should be 0 for E");
}


void t_ispunct_0x46()
{
    Assert(ispunct(70)  == 0 ,"ispunct should be 0 for F");
}


void t_ispunct_0x47()
{
    Assert(ispunct(71)  == 0 ,"ispunct should be 0 for G");
}


void t_ispunct_0x48()
{
    Assert(ispunct(72)  == 0 ,"ispunct should be 0 for H");
}


void t_ispunct_0x49()
{
    Assert(ispunct(73)  == 0 ,"ispunct should be 0 for I");
}


void t_ispunct_0x4a()
{
    Assert(ispunct(74)  == 0 ,"ispunct should be 0 for J");
}


void t_ispunct_0x4b()
{
    Assert(ispunct(75)  == 0 ,"ispunct should be 0 for K");
}


void t_ispunct_0x4c()
{
    Assert(ispunct(76)  == 0 ,"ispunct should be 0 for L");
}


void t_ispunct_0x4d()
{
    Assert(ispunct(77)  == 0 ,"ispunct should be 0 for M");
}


void t_ispunct_0x4e()
{
    Assert(ispunct(78)  == 0 ,"ispunct should be 0 for N");
}


void t_ispunct_0x4f()
{
    Assert(ispunct(79)  == 0 ,"ispunct should be 0 for O");
}


void t_ispunct_0x50()
{
    Assert(ispunct(80)  == 0 ,"ispunct should be 0 for P");
}


void t_ispunct_0x51()
{
    Assert(ispunct(81)  == 0 ,"ispunct should be 0 for Q");
}


void t_ispunct_0x52()
{
    Assert(ispunct(82)  == 0 ,"ispunct should be 0 for R");
}


void t_ispunct_0x53()
{
    Assert(ispunct(83)  == 0 ,"ispunct should be 0 for S");
}


void t_ispunct_0x54()
{
    Assert(ispunct(84)  == 0 ,"ispunct should be 0 for T");
}


void t_ispunct_0x55()
{
    Assert(ispunct(85)  == 0 ,"ispunct should be 0 for U");
}


void t_ispunct_0x56()
{
    Assert(ispunct(86)  == 0 ,"ispunct should be 0 for V");
}


void t_ispunct_0x57()
{
    Assert(ispunct(87)  == 0 ,"ispunct should be 0 for W");
}


void t_ispunct_0x58()
{
    Assert(ispunct(88)  == 0 ,"ispunct should be 0 for X");
}


void t_ispunct_0x59()
{
    Assert(ispunct(89)  == 0 ,"ispunct should be 0 for Y");
}


void t_ispunct_0x5a()
{
    Assert(ispunct(90)  == 0 ,"ispunct should be 0 for Z");
}


void t_ispunct_0x5b()
{
    Assert(ispunct(91) ,"ispunct should be 1 for [");
}


void t_ispunct_0x5c()
{
    Assert(ispunct(92) ,"ispunct should be 1 for 0x5c");
}


void t_ispunct_0x5d()
{
    Assert(ispunct(93) ,"ispunct should be 1 for ]");
}


void t_ispunct_0x5e()
{
    Assert(ispunct(94) ,"ispunct should be 1 for ^");
}


void t_ispunct_0x5f()
{
    Assert(ispunct(95) ,"ispunct should be 1 for _");
}


int main(void)
{
    suite_setup("test_ispunct_shard_02");
    suite_add_test(t_ispunct_0x40);
    suite_add_test(t_ispunct_0x41);
    suite_add_test(t_ispunct_0x42);
    suite_add_test(t_ispunct_0x43);
    suite_add_test(t_ispunct_0x44);
    suite_add_test(t_ispunct_0x45);
    suite_add_test(t_ispunct_0x46);
    suite_add_test(t_ispunct_0x47);
    suite_add_test(t_ispunct_0x48);
    suite_add_test(t_ispunct_0x49);
    suite_add_test(t_ispunct_0x4a);
    suite_add_test(t_ispunct_0x4b);
    suite_add_test(t_ispunct_0x4c);
    suite_add_test(t_ispunct_0x4d);
    suite_add_test(t_ispunct_0x4e);
    suite_add_test(t_ispunct_0x4f);
    suite_add_test(t_ispunct_0x50);
    suite_add_test(t_ispunct_0x51);
    suite_add_test(t_ispunct_0x52);
    suite_add_test(t_ispunct_0x53);
    suite_add_test(t_ispunct_0x54);
    suite_add_test(t_ispunct_0x55);
    suite_add_test(t_ispunct_0x56);
    suite_add_test(t_ispunct_0x57);
    suite_add_test(t_ispunct_0x58);
    suite_add_test(t_ispunct_0x59);
    suite_add_test(t_ispunct_0x5a);
    suite_add_test(t_ispunct_0x5b);
    suite_add_test(t_ispunct_0x5c);
    suite_add_test(t_ispunct_0x5d);
    suite_add_test(t_ispunct_0x5e);
    suite_add_test(t_ispunct_0x5f);
    return suite_run();
}
