/* auto-generated z88dk ctype shard */
#include "../../upstream/test/suites/ctype/ctype_test.h"

void t_isalpha_0x20()
{
    Assert(isalpha(32)  == 0 ,"isalpha should be 0 for  ");
}


void t_isalpha_0x21()
{
    Assert(isalpha(33)  == 0 ,"isalpha should be 0 for !");
}


void t_isalpha_0x22()
{
    Assert(isalpha(34)  == 0 ,"isalpha should be 0 for 0x22");
}


void t_isalpha_0x23()
{
    Assert(isalpha(35)  == 0 ,"isalpha should be 0 for #");
}


void t_isalpha_0x24()
{
    Assert(isalpha(36)  == 0 ,"isalpha should be 0 for $");
}


void t_isalpha_0x25()
{
    Assert(isalpha(37)  == 0 ,"isalpha should be 0 for %");
}


void t_isalpha_0x26()
{
    Assert(isalpha(38)  == 0 ,"isalpha should be 0 for &");
}


void t_isalpha_0x27()
{
    Assert(isalpha(39)  == 0 ,"isalpha should be 0 for '");
}


void t_isalpha_0x28()
{
    Assert(isalpha(40)  == 0 ,"isalpha should be 0 for (");
}


void t_isalpha_0x29()
{
    Assert(isalpha(41)  == 0 ,"isalpha should be 0 for )");
}


void t_isalpha_0x2a()
{
    Assert(isalpha(42)  == 0 ,"isalpha should be 0 for *");
}


void t_isalpha_0x2b()
{
    Assert(isalpha(43)  == 0 ,"isalpha should be 0 for +");
}


void t_isalpha_0x2c()
{
    Assert(isalpha(44)  == 0 ,"isalpha should be 0 for ,");
}


void t_isalpha_0x2d()
{
    Assert(isalpha(45)  == 0 ,"isalpha should be 0 for -");
}


void t_isalpha_0x2e()
{
    Assert(isalpha(46)  == 0 ,"isalpha should be 0 for .");
}


void t_isalpha_0x2f()
{
    Assert(isalpha(47)  == 0 ,"isalpha should be 0 for /");
}


void t_isalpha_0x30()
{
    Assert(isalpha(48)  == 0 ,"isalpha should be 0 for 0");
}


void t_isalpha_0x31()
{
    Assert(isalpha(49)  == 0 ,"isalpha should be 0 for 1");
}


void t_isalpha_0x32()
{
    Assert(isalpha(50)  == 0 ,"isalpha should be 0 for 2");
}


void t_isalpha_0x33()
{
    Assert(isalpha(51)  == 0 ,"isalpha should be 0 for 3");
}


void t_isalpha_0x34()
{
    Assert(isalpha(52)  == 0 ,"isalpha should be 0 for 4");
}


void t_isalpha_0x35()
{
    Assert(isalpha(53)  == 0 ,"isalpha should be 0 for 5");
}


void t_isalpha_0x36()
{
    Assert(isalpha(54)  == 0 ,"isalpha should be 0 for 6");
}


void t_isalpha_0x37()
{
    Assert(isalpha(55)  == 0 ,"isalpha should be 0 for 7");
}


void t_isalpha_0x38()
{
    Assert(isalpha(56)  == 0 ,"isalpha should be 0 for 8");
}


void t_isalpha_0x39()
{
    Assert(isalpha(57)  == 0 ,"isalpha should be 0 for 9");
}


void t_isalpha_0x3a()
{
    Assert(isalpha(58)  == 0 ,"isalpha should be 0 for :");
}


void t_isalpha_0x3b()
{
    Assert(isalpha(59)  == 0 ,"isalpha should be 0 for ;");
}


void t_isalpha_0x3c()
{
    Assert(isalpha(60)  == 0 ,"isalpha should be 0 for <");
}


void t_isalpha_0x3d()
{
    Assert(isalpha(61)  == 0 ,"isalpha should be 0 for =");
}


void t_isalpha_0x3e()
{
    Assert(isalpha(62)  == 0 ,"isalpha should be 0 for >");
}


void t_isalpha_0x3f()
{
    Assert(isalpha(63)  == 0 ,"isalpha should be 0 for ?");
}


int main(void)
{
    suite_setup("test_isalpha_shard_01");
    suite_add_test(t_isalpha_0x20);
    suite_add_test(t_isalpha_0x21);
    suite_add_test(t_isalpha_0x22);
    suite_add_test(t_isalpha_0x23);
    suite_add_test(t_isalpha_0x24);
    suite_add_test(t_isalpha_0x25);
    suite_add_test(t_isalpha_0x26);
    suite_add_test(t_isalpha_0x27);
    suite_add_test(t_isalpha_0x28);
    suite_add_test(t_isalpha_0x29);
    suite_add_test(t_isalpha_0x2a);
    suite_add_test(t_isalpha_0x2b);
    suite_add_test(t_isalpha_0x2c);
    suite_add_test(t_isalpha_0x2d);
    suite_add_test(t_isalpha_0x2e);
    suite_add_test(t_isalpha_0x2f);
    suite_add_test(t_isalpha_0x30);
    suite_add_test(t_isalpha_0x31);
    suite_add_test(t_isalpha_0x32);
    suite_add_test(t_isalpha_0x33);
    suite_add_test(t_isalpha_0x34);
    suite_add_test(t_isalpha_0x35);
    suite_add_test(t_isalpha_0x36);
    suite_add_test(t_isalpha_0x37);
    suite_add_test(t_isalpha_0x38);
    suite_add_test(t_isalpha_0x39);
    suite_add_test(t_isalpha_0x3a);
    suite_add_test(t_isalpha_0x3b);
    suite_add_test(t_isalpha_0x3c);
    suite_add_test(t_isalpha_0x3d);
    suite_add_test(t_isalpha_0x3e);
    suite_add_test(t_isalpha_0x3f);
    return suite_run();
}
