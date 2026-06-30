/* auto-generated z88dk ctype shard */
#include "../../upstream/test/suites/ctype/ctype_test.h"

void t_isspace_0x40()
{
    Assert(isspace(64)  == 0 ,"isspace should be 0 for @");
}


void t_isspace_0x41()
{
    Assert(isspace(65)  == 0 ,"isspace should be 0 for A");
}


void t_isspace_0x42()
{
    Assert(isspace(66)  == 0 ,"isspace should be 0 for B");
}


void t_isspace_0x43()
{
    Assert(isspace(67)  == 0 ,"isspace should be 0 for C");
}


void t_isspace_0x44()
{
    Assert(isspace(68)  == 0 ,"isspace should be 0 for D");
}


void t_isspace_0x45()
{
    Assert(isspace(69)  == 0 ,"isspace should be 0 for E");
}


void t_isspace_0x46()
{
    Assert(isspace(70)  == 0 ,"isspace should be 0 for F");
}


void t_isspace_0x47()
{
    Assert(isspace(71)  == 0 ,"isspace should be 0 for G");
}


void t_isspace_0x48()
{
    Assert(isspace(72)  == 0 ,"isspace should be 0 for H");
}


void t_isspace_0x49()
{
    Assert(isspace(73)  == 0 ,"isspace should be 0 for I");
}


void t_isspace_0x4a()
{
    Assert(isspace(74)  == 0 ,"isspace should be 0 for J");
}


void t_isspace_0x4b()
{
    Assert(isspace(75)  == 0 ,"isspace should be 0 for K");
}


void t_isspace_0x4c()
{
    Assert(isspace(76)  == 0 ,"isspace should be 0 for L");
}


void t_isspace_0x4d()
{
    Assert(isspace(77)  == 0 ,"isspace should be 0 for M");
}


void t_isspace_0x4e()
{
    Assert(isspace(78)  == 0 ,"isspace should be 0 for N");
}


void t_isspace_0x4f()
{
    Assert(isspace(79)  == 0 ,"isspace should be 0 for O");
}


void t_isspace_0x50()
{
    Assert(isspace(80)  == 0 ,"isspace should be 0 for P");
}


void t_isspace_0x51()
{
    Assert(isspace(81)  == 0 ,"isspace should be 0 for Q");
}


void t_isspace_0x52()
{
    Assert(isspace(82)  == 0 ,"isspace should be 0 for R");
}


void t_isspace_0x53()
{
    Assert(isspace(83)  == 0 ,"isspace should be 0 for S");
}


void t_isspace_0x54()
{
    Assert(isspace(84)  == 0 ,"isspace should be 0 for T");
}


void t_isspace_0x55()
{
    Assert(isspace(85)  == 0 ,"isspace should be 0 for U");
}


void t_isspace_0x56()
{
    Assert(isspace(86)  == 0 ,"isspace should be 0 for V");
}


void t_isspace_0x57()
{
    Assert(isspace(87)  == 0 ,"isspace should be 0 for W");
}


void t_isspace_0x58()
{
    Assert(isspace(88)  == 0 ,"isspace should be 0 for X");
}


void t_isspace_0x59()
{
    Assert(isspace(89)  == 0 ,"isspace should be 0 for Y");
}


void t_isspace_0x5a()
{
    Assert(isspace(90)  == 0 ,"isspace should be 0 for Z");
}


void t_isspace_0x5b()
{
    Assert(isspace(91)  == 0 ,"isspace should be 0 for [");
}


void t_isspace_0x5c()
{
    Assert(isspace(92)  == 0 ,"isspace should be 0 for 0x5c");
}


void t_isspace_0x5d()
{
    Assert(isspace(93)  == 0 ,"isspace should be 0 for ]");
}


void t_isspace_0x5e()
{
    Assert(isspace(94)  == 0 ,"isspace should be 0 for ^");
}


void t_isspace_0x5f()
{
    Assert(isspace(95)  == 0 ,"isspace should be 0 for _");
}


int main(void)
{
    suite_setup("test_isspace_shard_02");
    suite_add_test(t_isspace_0x40);
    suite_add_test(t_isspace_0x41);
    suite_add_test(t_isspace_0x42);
    suite_add_test(t_isspace_0x43);
    suite_add_test(t_isspace_0x44);
    suite_add_test(t_isspace_0x45);
    suite_add_test(t_isspace_0x46);
    suite_add_test(t_isspace_0x47);
    suite_add_test(t_isspace_0x48);
    suite_add_test(t_isspace_0x49);
    suite_add_test(t_isspace_0x4a);
    suite_add_test(t_isspace_0x4b);
    suite_add_test(t_isspace_0x4c);
    suite_add_test(t_isspace_0x4d);
    suite_add_test(t_isspace_0x4e);
    suite_add_test(t_isspace_0x4f);
    suite_add_test(t_isspace_0x50);
    suite_add_test(t_isspace_0x51);
    suite_add_test(t_isspace_0x52);
    suite_add_test(t_isspace_0x53);
    suite_add_test(t_isspace_0x54);
    suite_add_test(t_isspace_0x55);
    suite_add_test(t_isspace_0x56);
    suite_add_test(t_isspace_0x57);
    suite_add_test(t_isspace_0x58);
    suite_add_test(t_isspace_0x59);
    suite_add_test(t_isspace_0x5a);
    suite_add_test(t_isspace_0x5b);
    suite_add_test(t_isspace_0x5c);
    suite_add_test(t_isspace_0x5d);
    suite_add_test(t_isspace_0x5e);
    suite_add_test(t_isspace_0x5f);
    return suite_run();
}
