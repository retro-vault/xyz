/* auto-generated z88dk ctype shard */
#include "../../upstream/test/suites/ctype/ctype_test.h"

void t_isalnum_0x20()
{
    Assert(isalnum(32)  == 0 ,"isalnum should be 0 for  ");
}


void t_isalnum_0x21()
{
    Assert(isalnum(33)  == 0 ,"isalnum should be 0 for !");
}


void t_isalnum_0x22()
{
    Assert(isalnum(34)  == 0 ,"isalnum should be 0 for 0x22");
}


void t_isalnum_0x23()
{
    Assert(isalnum(35)  == 0 ,"isalnum should be 0 for #");
}


void t_isalnum_0x24()
{
    Assert(isalnum(36)  == 0 ,"isalnum should be 0 for $");
}


void t_isalnum_0x25()
{
    Assert(isalnum(37)  == 0 ,"isalnum should be 0 for %");
}


void t_isalnum_0x26()
{
    Assert(isalnum(38)  == 0 ,"isalnum should be 0 for &");
}


void t_isalnum_0x27()
{
    Assert(isalnum(39)  == 0 ,"isalnum should be 0 for '");
}


void t_isalnum_0x28()
{
    Assert(isalnum(40)  == 0 ,"isalnum should be 0 for (");
}


void t_isalnum_0x29()
{
    Assert(isalnum(41)  == 0 ,"isalnum should be 0 for )");
}


void t_isalnum_0x2a()
{
    Assert(isalnum(42)  == 0 ,"isalnum should be 0 for *");
}


void t_isalnum_0x2b()
{
    Assert(isalnum(43)  == 0 ,"isalnum should be 0 for +");
}


void t_isalnum_0x2c()
{
    Assert(isalnum(44)  == 0 ,"isalnum should be 0 for ,");
}


void t_isalnum_0x2d()
{
    Assert(isalnum(45)  == 0 ,"isalnum should be 0 for -");
}


void t_isalnum_0x2e()
{
    Assert(isalnum(46)  == 0 ,"isalnum should be 0 for .");
}


void t_isalnum_0x2f()
{
    Assert(isalnum(47)  == 0 ,"isalnum should be 0 for /");
}


void t_isalnum_0x30()
{
    Assert(isalnum(48) ,"isalnum should be 1 for 0");
}


void t_isalnum_0x31()
{
    Assert(isalnum(49) ,"isalnum should be 1 for 1");
}


void t_isalnum_0x32()
{
    Assert(isalnum(50) ,"isalnum should be 1 for 2");
}


void t_isalnum_0x33()
{
    Assert(isalnum(51) ,"isalnum should be 1 for 3");
}


void t_isalnum_0x34()
{
    Assert(isalnum(52) ,"isalnum should be 1 for 4");
}


void t_isalnum_0x35()
{
    Assert(isalnum(53) ,"isalnum should be 1 for 5");
}


void t_isalnum_0x36()
{
    Assert(isalnum(54) ,"isalnum should be 1 for 6");
}


void t_isalnum_0x37()
{
    Assert(isalnum(55) ,"isalnum should be 1 for 7");
}


void t_isalnum_0x38()
{
    Assert(isalnum(56) ,"isalnum should be 1 for 8");
}


void t_isalnum_0x39()
{
    Assert(isalnum(57) ,"isalnum should be 1 for 9");
}


void t_isalnum_0x3a()
{
    Assert(isalnum(58)  == 0 ,"isalnum should be 0 for :");
}


void t_isalnum_0x3b()
{
    Assert(isalnum(59)  == 0 ,"isalnum should be 0 for ;");
}


void t_isalnum_0x3c()
{
    Assert(isalnum(60)  == 0 ,"isalnum should be 0 for <");
}


void t_isalnum_0x3d()
{
    Assert(isalnum(61)  == 0 ,"isalnum should be 0 for =");
}


void t_isalnum_0x3e()
{
    Assert(isalnum(62)  == 0 ,"isalnum should be 0 for >");
}


void t_isalnum_0x3f()
{
    Assert(isalnum(63)  == 0 ,"isalnum should be 0 for ?");
}


int main(void)
{
    suite_setup("test_isalnum_shard_01");
    suite_add_test(t_isalnum_0x20);
    suite_add_test(t_isalnum_0x21);
    suite_add_test(t_isalnum_0x22);
    suite_add_test(t_isalnum_0x23);
    suite_add_test(t_isalnum_0x24);
    suite_add_test(t_isalnum_0x25);
    suite_add_test(t_isalnum_0x26);
    suite_add_test(t_isalnum_0x27);
    suite_add_test(t_isalnum_0x28);
    suite_add_test(t_isalnum_0x29);
    suite_add_test(t_isalnum_0x2a);
    suite_add_test(t_isalnum_0x2b);
    suite_add_test(t_isalnum_0x2c);
    suite_add_test(t_isalnum_0x2d);
    suite_add_test(t_isalnum_0x2e);
    suite_add_test(t_isalnum_0x2f);
    suite_add_test(t_isalnum_0x30);
    suite_add_test(t_isalnum_0x31);
    suite_add_test(t_isalnum_0x32);
    suite_add_test(t_isalnum_0x33);
    suite_add_test(t_isalnum_0x34);
    suite_add_test(t_isalnum_0x35);
    suite_add_test(t_isalnum_0x36);
    suite_add_test(t_isalnum_0x37);
    suite_add_test(t_isalnum_0x38);
    suite_add_test(t_isalnum_0x39);
    suite_add_test(t_isalnum_0x3a);
    suite_add_test(t_isalnum_0x3b);
    suite_add_test(t_isalnum_0x3c);
    suite_add_test(t_isalnum_0x3d);
    suite_add_test(t_isalnum_0x3e);
    suite_add_test(t_isalnum_0x3f);
    return suite_run();
}
