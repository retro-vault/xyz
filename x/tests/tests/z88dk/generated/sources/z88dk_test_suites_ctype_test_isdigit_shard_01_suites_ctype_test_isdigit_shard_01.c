/* auto-generated z88dk ctype shard */
#include "../../upstream/test/suites/ctype/ctype_test.h"

void t_isdigit_0x20()
{
    Assert(isdigit(32)  == 0 ,"isdigit should be 0 for  ");
}


void t_isdigit_0x21()
{
    Assert(isdigit(33)  == 0 ,"isdigit should be 0 for !");
}


void t_isdigit_0x22()
{
    Assert(isdigit(34)  == 0 ,"isdigit should be 0 for 0x22");
}


void t_isdigit_0x23()
{
    Assert(isdigit(35)  == 0 ,"isdigit should be 0 for #");
}


void t_isdigit_0x24()
{
    Assert(isdigit(36)  == 0 ,"isdigit should be 0 for $");
}


void t_isdigit_0x25()
{
    Assert(isdigit(37)  == 0 ,"isdigit should be 0 for %");
}


void t_isdigit_0x26()
{
    Assert(isdigit(38)  == 0 ,"isdigit should be 0 for &");
}


void t_isdigit_0x27()
{
    Assert(isdigit(39)  == 0 ,"isdigit should be 0 for '");
}


void t_isdigit_0x28()
{
    Assert(isdigit(40)  == 0 ,"isdigit should be 0 for (");
}


void t_isdigit_0x29()
{
    Assert(isdigit(41)  == 0 ,"isdigit should be 0 for )");
}


void t_isdigit_0x2a()
{
    Assert(isdigit(42)  == 0 ,"isdigit should be 0 for *");
}


void t_isdigit_0x2b()
{
    Assert(isdigit(43)  == 0 ,"isdigit should be 0 for +");
}


void t_isdigit_0x2c()
{
    Assert(isdigit(44)  == 0 ,"isdigit should be 0 for ,");
}


void t_isdigit_0x2d()
{
    Assert(isdigit(45)  == 0 ,"isdigit should be 0 for -");
}


void t_isdigit_0x2e()
{
    Assert(isdigit(46)  == 0 ,"isdigit should be 0 for .");
}


void t_isdigit_0x2f()
{
    Assert(isdigit(47)  == 0 ,"isdigit should be 0 for /");
}


void t_isdigit_0x30()
{
    Assert(isdigit(48) ,"isdigit should be 1 for 0");
}


void t_isdigit_0x31()
{
    Assert(isdigit(49) ,"isdigit should be 1 for 1");
}


void t_isdigit_0x32()
{
    Assert(isdigit(50) ,"isdigit should be 1 for 2");
}


void t_isdigit_0x33()
{
    Assert(isdigit(51) ,"isdigit should be 1 for 3");
}


void t_isdigit_0x34()
{
    Assert(isdigit(52) ,"isdigit should be 1 for 4");
}


void t_isdigit_0x35()
{
    Assert(isdigit(53) ,"isdigit should be 1 for 5");
}


void t_isdigit_0x36()
{
    Assert(isdigit(54) ,"isdigit should be 1 for 6");
}


void t_isdigit_0x37()
{
    Assert(isdigit(55) ,"isdigit should be 1 for 7");
}


void t_isdigit_0x38()
{
    Assert(isdigit(56) ,"isdigit should be 1 for 8");
}


void t_isdigit_0x39()
{
    Assert(isdigit(57) ,"isdigit should be 1 for 9");
}


void t_isdigit_0x3a()
{
    Assert(isdigit(58)  == 0 ,"isdigit should be 0 for :");
}


void t_isdigit_0x3b()
{
    Assert(isdigit(59)  == 0 ,"isdigit should be 0 for ;");
}


void t_isdigit_0x3c()
{
    Assert(isdigit(60)  == 0 ,"isdigit should be 0 for <");
}


void t_isdigit_0x3d()
{
    Assert(isdigit(61)  == 0 ,"isdigit should be 0 for =");
}


void t_isdigit_0x3e()
{
    Assert(isdigit(62)  == 0 ,"isdigit should be 0 for >");
}


void t_isdigit_0x3f()
{
    Assert(isdigit(63)  == 0 ,"isdigit should be 0 for ?");
}


int main(void)
{
    suite_setup("test_isdigit_shard_01");
    suite_add_test(t_isdigit_0x20);
    suite_add_test(t_isdigit_0x21);
    suite_add_test(t_isdigit_0x22);
    suite_add_test(t_isdigit_0x23);
    suite_add_test(t_isdigit_0x24);
    suite_add_test(t_isdigit_0x25);
    suite_add_test(t_isdigit_0x26);
    suite_add_test(t_isdigit_0x27);
    suite_add_test(t_isdigit_0x28);
    suite_add_test(t_isdigit_0x29);
    suite_add_test(t_isdigit_0x2a);
    suite_add_test(t_isdigit_0x2b);
    suite_add_test(t_isdigit_0x2c);
    suite_add_test(t_isdigit_0x2d);
    suite_add_test(t_isdigit_0x2e);
    suite_add_test(t_isdigit_0x2f);
    suite_add_test(t_isdigit_0x30);
    suite_add_test(t_isdigit_0x31);
    suite_add_test(t_isdigit_0x32);
    suite_add_test(t_isdigit_0x33);
    suite_add_test(t_isdigit_0x34);
    suite_add_test(t_isdigit_0x35);
    suite_add_test(t_isdigit_0x36);
    suite_add_test(t_isdigit_0x37);
    suite_add_test(t_isdigit_0x38);
    suite_add_test(t_isdigit_0x39);
    suite_add_test(t_isdigit_0x3a);
    suite_add_test(t_isdigit_0x3b);
    suite_add_test(t_isdigit_0x3c);
    suite_add_test(t_isdigit_0x3d);
    suite_add_test(t_isdigit_0x3e);
    suite_add_test(t_isdigit_0x3f);
    return suite_run();
}
