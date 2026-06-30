/* auto-generated z88dk ctype shard */
#include "../../upstream/test/suites/ctype/ctype_test.h"

void t_isgraph_0x20()
{
    Assert(isgraph(32)  == 0 ,"isgraph should be 0 for  ");
}


void t_isgraph_0x21()
{
    Assert(isgraph(33) ,"isgraph should be 1 for !");
}


void t_isgraph_0x22()
{
    Assert(isgraph(34) ,"isgraph should be 1 for 0x22");
}


void t_isgraph_0x23()
{
    Assert(isgraph(35) ,"isgraph should be 1 for #");
}


void t_isgraph_0x24()
{
    Assert(isgraph(36) ,"isgraph should be 1 for $");
}


void t_isgraph_0x25()
{
    Assert(isgraph(37) ,"isgraph should be 1 for %");
}


void t_isgraph_0x26()
{
    Assert(isgraph(38) ,"isgraph should be 1 for &");
}


void t_isgraph_0x27()
{
    Assert(isgraph(39) ,"isgraph should be 1 for '");
}


void t_isgraph_0x28()
{
    Assert(isgraph(40) ,"isgraph should be 1 for (");
}


void t_isgraph_0x29()
{
    Assert(isgraph(41) ,"isgraph should be 1 for )");
}


void t_isgraph_0x2a()
{
    Assert(isgraph(42) ,"isgraph should be 1 for *");
}


void t_isgraph_0x2b()
{
    Assert(isgraph(43) ,"isgraph should be 1 for +");
}


void t_isgraph_0x2c()
{
    Assert(isgraph(44) ,"isgraph should be 1 for ,");
}


void t_isgraph_0x2d()
{
    Assert(isgraph(45) ,"isgraph should be 1 for -");
}


void t_isgraph_0x2e()
{
    Assert(isgraph(46) ,"isgraph should be 1 for .");
}


void t_isgraph_0x2f()
{
    Assert(isgraph(47) ,"isgraph should be 1 for /");
}


void t_isgraph_0x30()
{
    Assert(isgraph(48) ,"isgraph should be 1 for 0");
}


void t_isgraph_0x31()
{
    Assert(isgraph(49) ,"isgraph should be 1 for 1");
}


void t_isgraph_0x32()
{
    Assert(isgraph(50) ,"isgraph should be 1 for 2");
}


void t_isgraph_0x33()
{
    Assert(isgraph(51) ,"isgraph should be 1 for 3");
}


void t_isgraph_0x34()
{
    Assert(isgraph(52) ,"isgraph should be 1 for 4");
}


void t_isgraph_0x35()
{
    Assert(isgraph(53) ,"isgraph should be 1 for 5");
}


void t_isgraph_0x36()
{
    Assert(isgraph(54) ,"isgraph should be 1 for 6");
}


void t_isgraph_0x37()
{
    Assert(isgraph(55) ,"isgraph should be 1 for 7");
}


void t_isgraph_0x38()
{
    Assert(isgraph(56) ,"isgraph should be 1 for 8");
}


void t_isgraph_0x39()
{
    Assert(isgraph(57) ,"isgraph should be 1 for 9");
}


void t_isgraph_0x3a()
{
    Assert(isgraph(58) ,"isgraph should be 1 for :");
}


void t_isgraph_0x3b()
{
    Assert(isgraph(59) ,"isgraph should be 1 for ;");
}


void t_isgraph_0x3c()
{
    Assert(isgraph(60) ,"isgraph should be 1 for <");
}


void t_isgraph_0x3d()
{
    Assert(isgraph(61) ,"isgraph should be 1 for =");
}


void t_isgraph_0x3e()
{
    Assert(isgraph(62) ,"isgraph should be 1 for >");
}


void t_isgraph_0x3f()
{
    Assert(isgraph(63) ,"isgraph should be 1 for ?");
}


int main(void)
{
    suite_setup("test_isgraph_shard_01");
    suite_add_test(t_isgraph_0x20);
    suite_add_test(t_isgraph_0x21);
    suite_add_test(t_isgraph_0x22);
    suite_add_test(t_isgraph_0x23);
    suite_add_test(t_isgraph_0x24);
    suite_add_test(t_isgraph_0x25);
    suite_add_test(t_isgraph_0x26);
    suite_add_test(t_isgraph_0x27);
    suite_add_test(t_isgraph_0x28);
    suite_add_test(t_isgraph_0x29);
    suite_add_test(t_isgraph_0x2a);
    suite_add_test(t_isgraph_0x2b);
    suite_add_test(t_isgraph_0x2c);
    suite_add_test(t_isgraph_0x2d);
    suite_add_test(t_isgraph_0x2e);
    suite_add_test(t_isgraph_0x2f);
    suite_add_test(t_isgraph_0x30);
    suite_add_test(t_isgraph_0x31);
    suite_add_test(t_isgraph_0x32);
    suite_add_test(t_isgraph_0x33);
    suite_add_test(t_isgraph_0x34);
    suite_add_test(t_isgraph_0x35);
    suite_add_test(t_isgraph_0x36);
    suite_add_test(t_isgraph_0x37);
    suite_add_test(t_isgraph_0x38);
    suite_add_test(t_isgraph_0x39);
    suite_add_test(t_isgraph_0x3a);
    suite_add_test(t_isgraph_0x3b);
    suite_add_test(t_isgraph_0x3c);
    suite_add_test(t_isgraph_0x3d);
    suite_add_test(t_isgraph_0x3e);
    suite_add_test(t_isgraph_0x3f);
    return suite_run();
}
