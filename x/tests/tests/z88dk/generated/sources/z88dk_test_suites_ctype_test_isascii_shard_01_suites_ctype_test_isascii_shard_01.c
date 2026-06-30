/* auto-generated z88dk ctype shard */
#include "../../upstream/test/suites/ctype/ctype_test.h"

void t_isascii_0x20()
{
    Assert(isascii(32) ,"isascii should be 1 for  ");
}


void t_isascii_0x21()
{
    Assert(isascii(33) ,"isascii should be 1 for !");
}


void t_isascii_0x22()
{
    Assert(isascii(34) ,"isascii should be 1 for 0x22");
}


void t_isascii_0x23()
{
    Assert(isascii(35) ,"isascii should be 1 for #");
}


void t_isascii_0x24()
{
    Assert(isascii(36) ,"isascii should be 1 for $");
}


void t_isascii_0x25()
{
    Assert(isascii(37) ,"isascii should be 1 for %");
}


void t_isascii_0x26()
{
    Assert(isascii(38) ,"isascii should be 1 for &");
}


void t_isascii_0x27()
{
    Assert(isascii(39) ,"isascii should be 1 for '");
}


void t_isascii_0x28()
{
    Assert(isascii(40) ,"isascii should be 1 for (");
}


void t_isascii_0x29()
{
    Assert(isascii(41) ,"isascii should be 1 for )");
}


void t_isascii_0x2a()
{
    Assert(isascii(42) ,"isascii should be 1 for *");
}


void t_isascii_0x2b()
{
    Assert(isascii(43) ,"isascii should be 1 for +");
}


void t_isascii_0x2c()
{
    Assert(isascii(44) ,"isascii should be 1 for ,");
}


void t_isascii_0x2d()
{
    Assert(isascii(45) ,"isascii should be 1 for -");
}


void t_isascii_0x2e()
{
    Assert(isascii(46) ,"isascii should be 1 for .");
}


void t_isascii_0x2f()
{
    Assert(isascii(47) ,"isascii should be 1 for /");
}


void t_isascii_0x30()
{
    Assert(isascii(48) ,"isascii should be 1 for 0");
}


void t_isascii_0x31()
{
    Assert(isascii(49) ,"isascii should be 1 for 1");
}


void t_isascii_0x32()
{
    Assert(isascii(50) ,"isascii should be 1 for 2");
}


void t_isascii_0x33()
{
    Assert(isascii(51) ,"isascii should be 1 for 3");
}


void t_isascii_0x34()
{
    Assert(isascii(52) ,"isascii should be 1 for 4");
}


void t_isascii_0x35()
{
    Assert(isascii(53) ,"isascii should be 1 for 5");
}


void t_isascii_0x36()
{
    Assert(isascii(54) ,"isascii should be 1 for 6");
}


void t_isascii_0x37()
{
    Assert(isascii(55) ,"isascii should be 1 for 7");
}


void t_isascii_0x38()
{
    Assert(isascii(56) ,"isascii should be 1 for 8");
}


void t_isascii_0x39()
{
    Assert(isascii(57) ,"isascii should be 1 for 9");
}


void t_isascii_0x3a()
{
    Assert(isascii(58) ,"isascii should be 1 for :");
}


void t_isascii_0x3b()
{
    Assert(isascii(59) ,"isascii should be 1 for ;");
}


void t_isascii_0x3c()
{
    Assert(isascii(60) ,"isascii should be 1 for <");
}


void t_isascii_0x3d()
{
    Assert(isascii(61) ,"isascii should be 1 for =");
}


void t_isascii_0x3e()
{
    Assert(isascii(62) ,"isascii should be 1 for >");
}


void t_isascii_0x3f()
{
    Assert(isascii(63) ,"isascii should be 1 for ?");
}


int main(void)
{
    suite_setup("test_isascii_shard_01");
    suite_add_test(t_isascii_0x20);
    suite_add_test(t_isascii_0x21);
    suite_add_test(t_isascii_0x22);
    suite_add_test(t_isascii_0x23);
    suite_add_test(t_isascii_0x24);
    suite_add_test(t_isascii_0x25);
    suite_add_test(t_isascii_0x26);
    suite_add_test(t_isascii_0x27);
    suite_add_test(t_isascii_0x28);
    suite_add_test(t_isascii_0x29);
    suite_add_test(t_isascii_0x2a);
    suite_add_test(t_isascii_0x2b);
    suite_add_test(t_isascii_0x2c);
    suite_add_test(t_isascii_0x2d);
    suite_add_test(t_isascii_0x2e);
    suite_add_test(t_isascii_0x2f);
    suite_add_test(t_isascii_0x30);
    suite_add_test(t_isascii_0x31);
    suite_add_test(t_isascii_0x32);
    suite_add_test(t_isascii_0x33);
    suite_add_test(t_isascii_0x34);
    suite_add_test(t_isascii_0x35);
    suite_add_test(t_isascii_0x36);
    suite_add_test(t_isascii_0x37);
    suite_add_test(t_isascii_0x38);
    suite_add_test(t_isascii_0x39);
    suite_add_test(t_isascii_0x3a);
    suite_add_test(t_isascii_0x3b);
    suite_add_test(t_isascii_0x3c);
    suite_add_test(t_isascii_0x3d);
    suite_add_test(t_isascii_0x3e);
    suite_add_test(t_isascii_0x3f);
    return suite_run();
}
