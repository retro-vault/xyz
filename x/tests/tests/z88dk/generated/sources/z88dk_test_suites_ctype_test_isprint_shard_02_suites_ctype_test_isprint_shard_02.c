/* auto-generated z88dk ctype shard */
#include "../../upstream/test/suites/ctype/ctype_test.h"

void t_isprint_0x40()
{
    Assert(isprint(64) ,"isprint should be 1 for @");
}


void t_isprint_0x41()
{
    Assert(isprint(65) ,"isprint should be 1 for A");
}


void t_isprint_0x42()
{
    Assert(isprint(66) ,"isprint should be 1 for B");
}


void t_isprint_0x43()
{
    Assert(isprint(67) ,"isprint should be 1 for C");
}


void t_isprint_0x44()
{
    Assert(isprint(68) ,"isprint should be 1 for D");
}


void t_isprint_0x45()
{
    Assert(isprint(69) ,"isprint should be 1 for E");
}


void t_isprint_0x46()
{
    Assert(isprint(70) ,"isprint should be 1 for F");
}


void t_isprint_0x47()
{
    Assert(isprint(71) ,"isprint should be 1 for G");
}


void t_isprint_0x48()
{
    Assert(isprint(72) ,"isprint should be 1 for H");
}


void t_isprint_0x49()
{
    Assert(isprint(73) ,"isprint should be 1 for I");
}


void t_isprint_0x4a()
{
    Assert(isprint(74) ,"isprint should be 1 for J");
}


void t_isprint_0x4b()
{
    Assert(isprint(75) ,"isprint should be 1 for K");
}


void t_isprint_0x4c()
{
    Assert(isprint(76) ,"isprint should be 1 for L");
}


void t_isprint_0x4d()
{
    Assert(isprint(77) ,"isprint should be 1 for M");
}


void t_isprint_0x4e()
{
    Assert(isprint(78) ,"isprint should be 1 for N");
}


void t_isprint_0x4f()
{
    Assert(isprint(79) ,"isprint should be 1 for O");
}


void t_isprint_0x50()
{
    Assert(isprint(80) ,"isprint should be 1 for P");
}


void t_isprint_0x51()
{
    Assert(isprint(81) ,"isprint should be 1 for Q");
}


void t_isprint_0x52()
{
    Assert(isprint(82) ,"isprint should be 1 for R");
}


void t_isprint_0x53()
{
    Assert(isprint(83) ,"isprint should be 1 for S");
}


void t_isprint_0x54()
{
    Assert(isprint(84) ,"isprint should be 1 for T");
}


void t_isprint_0x55()
{
    Assert(isprint(85) ,"isprint should be 1 for U");
}


void t_isprint_0x56()
{
    Assert(isprint(86) ,"isprint should be 1 for V");
}


void t_isprint_0x57()
{
    Assert(isprint(87) ,"isprint should be 1 for W");
}


void t_isprint_0x58()
{
    Assert(isprint(88) ,"isprint should be 1 for X");
}


void t_isprint_0x59()
{
    Assert(isprint(89) ,"isprint should be 1 for Y");
}


void t_isprint_0x5a()
{
    Assert(isprint(90) ,"isprint should be 1 for Z");
}


void t_isprint_0x5b()
{
    Assert(isprint(91) ,"isprint should be 1 for [");
}


void t_isprint_0x5c()
{
    Assert(isprint(92) ,"isprint should be 1 for 0x5c");
}


void t_isprint_0x5d()
{
    Assert(isprint(93) ,"isprint should be 1 for ]");
}


void t_isprint_0x5e()
{
    Assert(isprint(94) ,"isprint should be 1 for ^");
}


void t_isprint_0x5f()
{
    Assert(isprint(95) ,"isprint should be 1 for _");
}


int main(void)
{
    suite_setup("test_isprint_shard_02");
    suite_add_test(t_isprint_0x40);
    suite_add_test(t_isprint_0x41);
    suite_add_test(t_isprint_0x42);
    suite_add_test(t_isprint_0x43);
    suite_add_test(t_isprint_0x44);
    suite_add_test(t_isprint_0x45);
    suite_add_test(t_isprint_0x46);
    suite_add_test(t_isprint_0x47);
    suite_add_test(t_isprint_0x48);
    suite_add_test(t_isprint_0x49);
    suite_add_test(t_isprint_0x4a);
    suite_add_test(t_isprint_0x4b);
    suite_add_test(t_isprint_0x4c);
    suite_add_test(t_isprint_0x4d);
    suite_add_test(t_isprint_0x4e);
    suite_add_test(t_isprint_0x4f);
    suite_add_test(t_isprint_0x50);
    suite_add_test(t_isprint_0x51);
    suite_add_test(t_isprint_0x52);
    suite_add_test(t_isprint_0x53);
    suite_add_test(t_isprint_0x54);
    suite_add_test(t_isprint_0x55);
    suite_add_test(t_isprint_0x56);
    suite_add_test(t_isprint_0x57);
    suite_add_test(t_isprint_0x58);
    suite_add_test(t_isprint_0x59);
    suite_add_test(t_isprint_0x5a);
    suite_add_test(t_isprint_0x5b);
    suite_add_test(t_isprint_0x5c);
    suite_add_test(t_isprint_0x5d);
    suite_add_test(t_isprint_0x5e);
    suite_add_test(t_isprint_0x5f);
    return suite_run();
}
