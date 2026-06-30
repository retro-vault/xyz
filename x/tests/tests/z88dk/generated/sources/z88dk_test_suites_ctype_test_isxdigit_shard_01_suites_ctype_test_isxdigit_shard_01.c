/* auto-generated z88dk ctype shard */
#include "../../upstream/test/suites/ctype/ctype_test.h"

void t_isxdigit_0x20()
{
    Assert(isxdigit(32)  == 0 ,"isxdigit should be 0 for  ");
}


void t_isxdigit_0x21()
{
    Assert(isxdigit(33)  == 0 ,"isxdigit should be 0 for !");
}


void t_isxdigit_0x22()
{
    Assert(isxdigit(34)  == 0 ,"isxdigit should be 0 for 0x22");
}


void t_isxdigit_0x23()
{
    Assert(isxdigit(35)  == 0 ,"isxdigit should be 0 for #");
}


void t_isxdigit_0x24()
{
    Assert(isxdigit(36)  == 0 ,"isxdigit should be 0 for $");
}


void t_isxdigit_0x25()
{
    Assert(isxdigit(37)  == 0 ,"isxdigit should be 0 for %");
}


void t_isxdigit_0x26()
{
    Assert(isxdigit(38)  == 0 ,"isxdigit should be 0 for &");
}


void t_isxdigit_0x27()
{
    Assert(isxdigit(39)  == 0 ,"isxdigit should be 0 for '");
}


void t_isxdigit_0x28()
{
    Assert(isxdigit(40)  == 0 ,"isxdigit should be 0 for (");
}


void t_isxdigit_0x29()
{
    Assert(isxdigit(41)  == 0 ,"isxdigit should be 0 for )");
}


void t_isxdigit_0x2a()
{
    Assert(isxdigit(42)  == 0 ,"isxdigit should be 0 for *");
}


void t_isxdigit_0x2b()
{
    Assert(isxdigit(43)  == 0 ,"isxdigit should be 0 for +");
}


void t_isxdigit_0x2c()
{
    Assert(isxdigit(44)  == 0 ,"isxdigit should be 0 for ,");
}


void t_isxdigit_0x2d()
{
    Assert(isxdigit(45)  == 0 ,"isxdigit should be 0 for -");
}


void t_isxdigit_0x2e()
{
    Assert(isxdigit(46)  == 0 ,"isxdigit should be 0 for .");
}


void t_isxdigit_0x2f()
{
    Assert(isxdigit(47)  == 0 ,"isxdigit should be 0 for /");
}


void t_isxdigit_0x30()
{
    Assert(isxdigit(48) ,"isxdigit should be 1 for 0");
}


void t_isxdigit_0x31()
{
    Assert(isxdigit(49) ,"isxdigit should be 1 for 1");
}


void t_isxdigit_0x32()
{
    Assert(isxdigit(50) ,"isxdigit should be 1 for 2");
}


void t_isxdigit_0x33()
{
    Assert(isxdigit(51) ,"isxdigit should be 1 for 3");
}


void t_isxdigit_0x34()
{
    Assert(isxdigit(52) ,"isxdigit should be 1 for 4");
}


void t_isxdigit_0x35()
{
    Assert(isxdigit(53) ,"isxdigit should be 1 for 5");
}


void t_isxdigit_0x36()
{
    Assert(isxdigit(54) ,"isxdigit should be 1 for 6");
}


void t_isxdigit_0x37()
{
    Assert(isxdigit(55) ,"isxdigit should be 1 for 7");
}


void t_isxdigit_0x38()
{
    Assert(isxdigit(56) ,"isxdigit should be 1 for 8");
}


void t_isxdigit_0x39()
{
    Assert(isxdigit(57) ,"isxdigit should be 1 for 9");
}


void t_isxdigit_0x3a()
{
    Assert(isxdigit(58)  == 0 ,"isxdigit should be 0 for :");
}


void t_isxdigit_0x3b()
{
    Assert(isxdigit(59)  == 0 ,"isxdigit should be 0 for ;");
}


void t_isxdigit_0x3c()
{
    Assert(isxdigit(60)  == 0 ,"isxdigit should be 0 for <");
}


void t_isxdigit_0x3d()
{
    Assert(isxdigit(61)  == 0 ,"isxdigit should be 0 for =");
}


void t_isxdigit_0x3e()
{
    Assert(isxdigit(62)  == 0 ,"isxdigit should be 0 for >");
}


void t_isxdigit_0x3f()
{
    Assert(isxdigit(63)  == 0 ,"isxdigit should be 0 for ?");
}


int main(void)
{
    suite_setup("test_isxdigit_shard_01");
    suite_add_test(t_isxdigit_0x20);
    suite_add_test(t_isxdigit_0x21);
    suite_add_test(t_isxdigit_0x22);
    suite_add_test(t_isxdigit_0x23);
    suite_add_test(t_isxdigit_0x24);
    suite_add_test(t_isxdigit_0x25);
    suite_add_test(t_isxdigit_0x26);
    suite_add_test(t_isxdigit_0x27);
    suite_add_test(t_isxdigit_0x28);
    suite_add_test(t_isxdigit_0x29);
    suite_add_test(t_isxdigit_0x2a);
    suite_add_test(t_isxdigit_0x2b);
    suite_add_test(t_isxdigit_0x2c);
    suite_add_test(t_isxdigit_0x2d);
    suite_add_test(t_isxdigit_0x2e);
    suite_add_test(t_isxdigit_0x2f);
    suite_add_test(t_isxdigit_0x30);
    suite_add_test(t_isxdigit_0x31);
    suite_add_test(t_isxdigit_0x32);
    suite_add_test(t_isxdigit_0x33);
    suite_add_test(t_isxdigit_0x34);
    suite_add_test(t_isxdigit_0x35);
    suite_add_test(t_isxdigit_0x36);
    suite_add_test(t_isxdigit_0x37);
    suite_add_test(t_isxdigit_0x38);
    suite_add_test(t_isxdigit_0x39);
    suite_add_test(t_isxdigit_0x3a);
    suite_add_test(t_isxdigit_0x3b);
    suite_add_test(t_isxdigit_0x3c);
    suite_add_test(t_isxdigit_0x3d);
    suite_add_test(t_isxdigit_0x3e);
    suite_add_test(t_isxdigit_0x3f);
    return suite_run();
}
