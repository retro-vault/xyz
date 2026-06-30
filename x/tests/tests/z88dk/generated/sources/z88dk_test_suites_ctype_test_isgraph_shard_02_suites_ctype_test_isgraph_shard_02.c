/* auto-generated z88dk ctype shard */
#include "../../upstream/test/suites/ctype/ctype_test.h"

void t_isgraph_0x40()
{
    Assert(isgraph(64) ,"isgraph should be 1 for @");
}


void t_isgraph_0x41()
{
    Assert(isgraph(65) ,"isgraph should be 1 for A");
}


void t_isgraph_0x42()
{
    Assert(isgraph(66) ,"isgraph should be 1 for B");
}


void t_isgraph_0x43()
{
    Assert(isgraph(67) ,"isgraph should be 1 for C");
}


void t_isgraph_0x44()
{
    Assert(isgraph(68) ,"isgraph should be 1 for D");
}


void t_isgraph_0x45()
{
    Assert(isgraph(69) ,"isgraph should be 1 for E");
}


void t_isgraph_0x46()
{
    Assert(isgraph(70) ,"isgraph should be 1 for F");
}


void t_isgraph_0x47()
{
    Assert(isgraph(71) ,"isgraph should be 1 for G");
}


void t_isgraph_0x48()
{
    Assert(isgraph(72) ,"isgraph should be 1 for H");
}


void t_isgraph_0x49()
{
    Assert(isgraph(73) ,"isgraph should be 1 for I");
}


void t_isgraph_0x4a()
{
    Assert(isgraph(74) ,"isgraph should be 1 for J");
}


void t_isgraph_0x4b()
{
    Assert(isgraph(75) ,"isgraph should be 1 for K");
}


void t_isgraph_0x4c()
{
    Assert(isgraph(76) ,"isgraph should be 1 for L");
}


void t_isgraph_0x4d()
{
    Assert(isgraph(77) ,"isgraph should be 1 for M");
}


void t_isgraph_0x4e()
{
    Assert(isgraph(78) ,"isgraph should be 1 for N");
}


void t_isgraph_0x4f()
{
    Assert(isgraph(79) ,"isgraph should be 1 for O");
}


void t_isgraph_0x50()
{
    Assert(isgraph(80) ,"isgraph should be 1 for P");
}


void t_isgraph_0x51()
{
    Assert(isgraph(81) ,"isgraph should be 1 for Q");
}


void t_isgraph_0x52()
{
    Assert(isgraph(82) ,"isgraph should be 1 for R");
}


void t_isgraph_0x53()
{
    Assert(isgraph(83) ,"isgraph should be 1 for S");
}


void t_isgraph_0x54()
{
    Assert(isgraph(84) ,"isgraph should be 1 for T");
}


void t_isgraph_0x55()
{
    Assert(isgraph(85) ,"isgraph should be 1 for U");
}


void t_isgraph_0x56()
{
    Assert(isgraph(86) ,"isgraph should be 1 for V");
}


void t_isgraph_0x57()
{
    Assert(isgraph(87) ,"isgraph should be 1 for W");
}


void t_isgraph_0x58()
{
    Assert(isgraph(88) ,"isgraph should be 1 for X");
}


void t_isgraph_0x59()
{
    Assert(isgraph(89) ,"isgraph should be 1 for Y");
}


void t_isgraph_0x5a()
{
    Assert(isgraph(90) ,"isgraph should be 1 for Z");
}


void t_isgraph_0x5b()
{
    Assert(isgraph(91) ,"isgraph should be 1 for [");
}


void t_isgraph_0x5c()
{
    Assert(isgraph(92) ,"isgraph should be 1 for 0x5c");
}


void t_isgraph_0x5d()
{
    Assert(isgraph(93) ,"isgraph should be 1 for ]");
}


void t_isgraph_0x5e()
{
    Assert(isgraph(94) ,"isgraph should be 1 for ^");
}


void t_isgraph_0x5f()
{
    Assert(isgraph(95) ,"isgraph should be 1 for _");
}


int main(void)
{
    suite_setup("test_isgraph_shard_02");
    suite_add_test(t_isgraph_0x40);
    suite_add_test(t_isgraph_0x41);
    suite_add_test(t_isgraph_0x42);
    suite_add_test(t_isgraph_0x43);
    suite_add_test(t_isgraph_0x44);
    suite_add_test(t_isgraph_0x45);
    suite_add_test(t_isgraph_0x46);
    suite_add_test(t_isgraph_0x47);
    suite_add_test(t_isgraph_0x48);
    suite_add_test(t_isgraph_0x49);
    suite_add_test(t_isgraph_0x4a);
    suite_add_test(t_isgraph_0x4b);
    suite_add_test(t_isgraph_0x4c);
    suite_add_test(t_isgraph_0x4d);
    suite_add_test(t_isgraph_0x4e);
    suite_add_test(t_isgraph_0x4f);
    suite_add_test(t_isgraph_0x50);
    suite_add_test(t_isgraph_0x51);
    suite_add_test(t_isgraph_0x52);
    suite_add_test(t_isgraph_0x53);
    suite_add_test(t_isgraph_0x54);
    suite_add_test(t_isgraph_0x55);
    suite_add_test(t_isgraph_0x56);
    suite_add_test(t_isgraph_0x57);
    suite_add_test(t_isgraph_0x58);
    suite_add_test(t_isgraph_0x59);
    suite_add_test(t_isgraph_0x5a);
    suite_add_test(t_isgraph_0x5b);
    suite_add_test(t_isgraph_0x5c);
    suite_add_test(t_isgraph_0x5d);
    suite_add_test(t_isgraph_0x5e);
    suite_add_test(t_isgraph_0x5f);
    return suite_run();
}
