/* auto-generated z88dk ctype shard */
#include "../../upstream/test/suites/ctype/ctype_test.h"

void t_ispunct_0x20()
{
    Assert(ispunct(32)  == 0 ,"ispunct should be 0 for  ");
}


void t_ispunct_0x21()
{
    Assert(ispunct(33) ,"ispunct should be 1 for !");
}


void t_ispunct_0x22()
{
    Assert(ispunct(34) ,"ispunct should be 1 for 0x22");
}


void t_ispunct_0x23()
{
    Assert(ispunct(35) ,"ispunct should be 1 for #");
}


void t_ispunct_0x24()
{
    Assert(ispunct(36) ,"ispunct should be 1 for $");
}


void t_ispunct_0x25()
{
    Assert(ispunct(37) ,"ispunct should be 1 for %");
}


void t_ispunct_0x26()
{
    Assert(ispunct(38) ,"ispunct should be 1 for &");
}


void t_ispunct_0x27()
{
    Assert(ispunct(39) ,"ispunct should be 1 for '");
}


void t_ispunct_0x28()
{
    Assert(ispunct(40) ,"ispunct should be 1 for (");
}


void t_ispunct_0x29()
{
    Assert(ispunct(41) ,"ispunct should be 1 for )");
}


void t_ispunct_0x2a()
{
    Assert(ispunct(42) ,"ispunct should be 1 for *");
}


void t_ispunct_0x2b()
{
    Assert(ispunct(43) ,"ispunct should be 1 for +");
}


void t_ispunct_0x2c()
{
    Assert(ispunct(44) ,"ispunct should be 1 for ,");
}


void t_ispunct_0x2d()
{
    Assert(ispunct(45) ,"ispunct should be 1 for -");
}


void t_ispunct_0x2e()
{
    Assert(ispunct(46) ,"ispunct should be 1 for .");
}


void t_ispunct_0x2f()
{
    Assert(ispunct(47) ,"ispunct should be 1 for /");
}


void t_ispunct_0x30()
{
    Assert(ispunct(48)  == 0 ,"ispunct should be 0 for 0");
}


void t_ispunct_0x31()
{
    Assert(ispunct(49)  == 0 ,"ispunct should be 0 for 1");
}


void t_ispunct_0x32()
{
    Assert(ispunct(50)  == 0 ,"ispunct should be 0 for 2");
}


void t_ispunct_0x33()
{
    Assert(ispunct(51)  == 0 ,"ispunct should be 0 for 3");
}


void t_ispunct_0x34()
{
    Assert(ispunct(52)  == 0 ,"ispunct should be 0 for 4");
}


void t_ispunct_0x35()
{
    Assert(ispunct(53)  == 0 ,"ispunct should be 0 for 5");
}


void t_ispunct_0x36()
{
    Assert(ispunct(54)  == 0 ,"ispunct should be 0 for 6");
}


void t_ispunct_0x37()
{
    Assert(ispunct(55)  == 0 ,"ispunct should be 0 for 7");
}


void t_ispunct_0x38()
{
    Assert(ispunct(56)  == 0 ,"ispunct should be 0 for 8");
}


void t_ispunct_0x39()
{
    Assert(ispunct(57)  == 0 ,"ispunct should be 0 for 9");
}


void t_ispunct_0x3a()
{
    Assert(ispunct(58) ,"ispunct should be 1 for :");
}


void t_ispunct_0x3b()
{
    Assert(ispunct(59) ,"ispunct should be 1 for ;");
}


void t_ispunct_0x3c()
{
    Assert(ispunct(60) ,"ispunct should be 1 for <");
}


void t_ispunct_0x3d()
{
    Assert(ispunct(61) ,"ispunct should be 1 for =");
}


void t_ispunct_0x3e()
{
    Assert(ispunct(62) ,"ispunct should be 1 for >");
}


void t_ispunct_0x3f()
{
    Assert(ispunct(63) ,"ispunct should be 1 for ?");
}


int main(void)
{
    suite_setup("test_ispunct_shard_01");
    suite_add_test(t_ispunct_0x20);
    suite_add_test(t_ispunct_0x21);
    suite_add_test(t_ispunct_0x22);
    suite_add_test(t_ispunct_0x23);
    suite_add_test(t_ispunct_0x24);
    suite_add_test(t_ispunct_0x25);
    suite_add_test(t_ispunct_0x26);
    suite_add_test(t_ispunct_0x27);
    suite_add_test(t_ispunct_0x28);
    suite_add_test(t_ispunct_0x29);
    suite_add_test(t_ispunct_0x2a);
    suite_add_test(t_ispunct_0x2b);
    suite_add_test(t_ispunct_0x2c);
    suite_add_test(t_ispunct_0x2d);
    suite_add_test(t_ispunct_0x2e);
    suite_add_test(t_ispunct_0x2f);
    suite_add_test(t_ispunct_0x30);
    suite_add_test(t_ispunct_0x31);
    suite_add_test(t_ispunct_0x32);
    suite_add_test(t_ispunct_0x33);
    suite_add_test(t_ispunct_0x34);
    suite_add_test(t_ispunct_0x35);
    suite_add_test(t_ispunct_0x36);
    suite_add_test(t_ispunct_0x37);
    suite_add_test(t_ispunct_0x38);
    suite_add_test(t_ispunct_0x39);
    suite_add_test(t_ispunct_0x3a);
    suite_add_test(t_ispunct_0x3b);
    suite_add_test(t_ispunct_0x3c);
    suite_add_test(t_ispunct_0x3d);
    suite_add_test(t_ispunct_0x3e);
    suite_add_test(t_ispunct_0x3f);
    return suite_run();
}
