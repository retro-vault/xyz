/* auto-generated z88dk ctype shard */
#include "../../upstream/test/suites/ctype/ctype_test.h"

void t_isprint_0x20()
{
    Assert(isprint(32) ,"isprint should be 1 for  ");
}


void t_isprint_0x21()
{
    Assert(isprint(33) ,"isprint should be 1 for !");
}


void t_isprint_0x22()
{
    Assert(isprint(34) ,"isprint should be 1 for 0x22");
}


void t_isprint_0x23()
{
    Assert(isprint(35) ,"isprint should be 1 for #");
}


void t_isprint_0x24()
{
    Assert(isprint(36) ,"isprint should be 1 for $");
}


void t_isprint_0x25()
{
    Assert(isprint(37) ,"isprint should be 1 for %");
}


void t_isprint_0x26()
{
    Assert(isprint(38) ,"isprint should be 1 for &");
}


void t_isprint_0x27()
{
    Assert(isprint(39) ,"isprint should be 1 for '");
}


void t_isprint_0x28()
{
    Assert(isprint(40) ,"isprint should be 1 for (");
}


void t_isprint_0x29()
{
    Assert(isprint(41) ,"isprint should be 1 for )");
}


void t_isprint_0x2a()
{
    Assert(isprint(42) ,"isprint should be 1 for *");
}


void t_isprint_0x2b()
{
    Assert(isprint(43) ,"isprint should be 1 for +");
}


void t_isprint_0x2c()
{
    Assert(isprint(44) ,"isprint should be 1 for ,");
}


void t_isprint_0x2d()
{
    Assert(isprint(45) ,"isprint should be 1 for -");
}


void t_isprint_0x2e()
{
    Assert(isprint(46) ,"isprint should be 1 for .");
}


void t_isprint_0x2f()
{
    Assert(isprint(47) ,"isprint should be 1 for /");
}


void t_isprint_0x30()
{
    Assert(isprint(48) ,"isprint should be 1 for 0");
}


void t_isprint_0x31()
{
    Assert(isprint(49) ,"isprint should be 1 for 1");
}


void t_isprint_0x32()
{
    Assert(isprint(50) ,"isprint should be 1 for 2");
}


void t_isprint_0x33()
{
    Assert(isprint(51) ,"isprint should be 1 for 3");
}


void t_isprint_0x34()
{
    Assert(isprint(52) ,"isprint should be 1 for 4");
}


void t_isprint_0x35()
{
    Assert(isprint(53) ,"isprint should be 1 for 5");
}


void t_isprint_0x36()
{
    Assert(isprint(54) ,"isprint should be 1 for 6");
}


void t_isprint_0x37()
{
    Assert(isprint(55) ,"isprint should be 1 for 7");
}


void t_isprint_0x38()
{
    Assert(isprint(56) ,"isprint should be 1 for 8");
}


void t_isprint_0x39()
{
    Assert(isprint(57) ,"isprint should be 1 for 9");
}


void t_isprint_0x3a()
{
    Assert(isprint(58) ,"isprint should be 1 for :");
}


void t_isprint_0x3b()
{
    Assert(isprint(59) ,"isprint should be 1 for ;");
}


void t_isprint_0x3c()
{
    Assert(isprint(60) ,"isprint should be 1 for <");
}


void t_isprint_0x3d()
{
    Assert(isprint(61) ,"isprint should be 1 for =");
}


void t_isprint_0x3e()
{
    Assert(isprint(62) ,"isprint should be 1 for >");
}


void t_isprint_0x3f()
{
    Assert(isprint(63) ,"isprint should be 1 for ?");
}


int main(void)
{
    suite_setup("test_isprint_shard_01");
    suite_add_test(t_isprint_0x20);
    suite_add_test(t_isprint_0x21);
    suite_add_test(t_isprint_0x22);
    suite_add_test(t_isprint_0x23);
    suite_add_test(t_isprint_0x24);
    suite_add_test(t_isprint_0x25);
    suite_add_test(t_isprint_0x26);
    suite_add_test(t_isprint_0x27);
    suite_add_test(t_isprint_0x28);
    suite_add_test(t_isprint_0x29);
    suite_add_test(t_isprint_0x2a);
    suite_add_test(t_isprint_0x2b);
    suite_add_test(t_isprint_0x2c);
    suite_add_test(t_isprint_0x2d);
    suite_add_test(t_isprint_0x2e);
    suite_add_test(t_isprint_0x2f);
    suite_add_test(t_isprint_0x30);
    suite_add_test(t_isprint_0x31);
    suite_add_test(t_isprint_0x32);
    suite_add_test(t_isprint_0x33);
    suite_add_test(t_isprint_0x34);
    suite_add_test(t_isprint_0x35);
    suite_add_test(t_isprint_0x36);
    suite_add_test(t_isprint_0x37);
    suite_add_test(t_isprint_0x38);
    suite_add_test(t_isprint_0x39);
    suite_add_test(t_isprint_0x3a);
    suite_add_test(t_isprint_0x3b);
    suite_add_test(t_isprint_0x3c);
    suite_add_test(t_isprint_0x3d);
    suite_add_test(t_isprint_0x3e);
    suite_add_test(t_isprint_0x3f);
    return suite_run();
}
