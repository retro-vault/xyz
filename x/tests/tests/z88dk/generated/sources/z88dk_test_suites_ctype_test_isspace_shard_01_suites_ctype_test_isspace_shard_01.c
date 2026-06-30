/* auto-generated z88dk ctype shard */
#include "../../upstream/test/suites/ctype/ctype_test.h"

void t_isspace_0x20()
{
    Assert(isspace(32) ,"isspace should be 1 for  ");
}


void t_isspace_0x21()
{
    Assert(isspace(33)  == 0 ,"isspace should be 0 for !");
}


void t_isspace_0x22()
{
    Assert(isspace(34)  == 0 ,"isspace should be 0 for 0x22");
}


void t_isspace_0x23()
{
    Assert(isspace(35)  == 0 ,"isspace should be 0 for #");
}


void t_isspace_0x24()
{
    Assert(isspace(36)  == 0 ,"isspace should be 0 for $");
}


void t_isspace_0x25()
{
    Assert(isspace(37)  == 0 ,"isspace should be 0 for %");
}


void t_isspace_0x26()
{
    Assert(isspace(38)  == 0 ,"isspace should be 0 for &");
}


void t_isspace_0x27()
{
    Assert(isspace(39)  == 0 ,"isspace should be 0 for '");
}


void t_isspace_0x28()
{
    Assert(isspace(40)  == 0 ,"isspace should be 0 for (");
}


void t_isspace_0x29()
{
    Assert(isspace(41)  == 0 ,"isspace should be 0 for )");
}


void t_isspace_0x2a()
{
    Assert(isspace(42)  == 0 ,"isspace should be 0 for *");
}


void t_isspace_0x2b()
{
    Assert(isspace(43)  == 0 ,"isspace should be 0 for +");
}


void t_isspace_0x2c()
{
    Assert(isspace(44)  == 0 ,"isspace should be 0 for ,");
}


void t_isspace_0x2d()
{
    Assert(isspace(45)  == 0 ,"isspace should be 0 for -");
}


void t_isspace_0x2e()
{
    Assert(isspace(46)  == 0 ,"isspace should be 0 for .");
}


void t_isspace_0x2f()
{
    Assert(isspace(47)  == 0 ,"isspace should be 0 for /");
}


void t_isspace_0x30()
{
    Assert(isspace(48)  == 0 ,"isspace should be 0 for 0");
}


void t_isspace_0x31()
{
    Assert(isspace(49)  == 0 ,"isspace should be 0 for 1");
}


void t_isspace_0x32()
{
    Assert(isspace(50)  == 0 ,"isspace should be 0 for 2");
}


void t_isspace_0x33()
{
    Assert(isspace(51)  == 0 ,"isspace should be 0 for 3");
}


void t_isspace_0x34()
{
    Assert(isspace(52)  == 0 ,"isspace should be 0 for 4");
}


void t_isspace_0x35()
{
    Assert(isspace(53)  == 0 ,"isspace should be 0 for 5");
}


void t_isspace_0x36()
{
    Assert(isspace(54)  == 0 ,"isspace should be 0 for 6");
}


void t_isspace_0x37()
{
    Assert(isspace(55)  == 0 ,"isspace should be 0 for 7");
}


void t_isspace_0x38()
{
    Assert(isspace(56)  == 0 ,"isspace should be 0 for 8");
}


void t_isspace_0x39()
{
    Assert(isspace(57)  == 0 ,"isspace should be 0 for 9");
}


void t_isspace_0x3a()
{
    Assert(isspace(58)  == 0 ,"isspace should be 0 for :");
}


void t_isspace_0x3b()
{
    Assert(isspace(59)  == 0 ,"isspace should be 0 for ;");
}


void t_isspace_0x3c()
{
    Assert(isspace(60)  == 0 ,"isspace should be 0 for <");
}


void t_isspace_0x3d()
{
    Assert(isspace(61)  == 0 ,"isspace should be 0 for =");
}


void t_isspace_0x3e()
{
    Assert(isspace(62)  == 0 ,"isspace should be 0 for >");
}


void t_isspace_0x3f()
{
    Assert(isspace(63)  == 0 ,"isspace should be 0 for ?");
}


int main(void)
{
    suite_setup("test_isspace_shard_01");
    suite_add_test(t_isspace_0x20);
    suite_add_test(t_isspace_0x21);
    suite_add_test(t_isspace_0x22);
    suite_add_test(t_isspace_0x23);
    suite_add_test(t_isspace_0x24);
    suite_add_test(t_isspace_0x25);
    suite_add_test(t_isspace_0x26);
    suite_add_test(t_isspace_0x27);
    suite_add_test(t_isspace_0x28);
    suite_add_test(t_isspace_0x29);
    suite_add_test(t_isspace_0x2a);
    suite_add_test(t_isspace_0x2b);
    suite_add_test(t_isspace_0x2c);
    suite_add_test(t_isspace_0x2d);
    suite_add_test(t_isspace_0x2e);
    suite_add_test(t_isspace_0x2f);
    suite_add_test(t_isspace_0x30);
    suite_add_test(t_isspace_0x31);
    suite_add_test(t_isspace_0x32);
    suite_add_test(t_isspace_0x33);
    suite_add_test(t_isspace_0x34);
    suite_add_test(t_isspace_0x35);
    suite_add_test(t_isspace_0x36);
    suite_add_test(t_isspace_0x37);
    suite_add_test(t_isspace_0x38);
    suite_add_test(t_isspace_0x39);
    suite_add_test(t_isspace_0x3a);
    suite_add_test(t_isspace_0x3b);
    suite_add_test(t_isspace_0x3c);
    suite_add_test(t_isspace_0x3d);
    suite_add_test(t_isspace_0x3e);
    suite_add_test(t_isspace_0x3f);
    return suite_run();
}
