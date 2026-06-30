/* auto-generated z88dk ctype shard */
#include "../../upstream/test/suites/ctype/ctype_test.h"

void t_iscntrl_0x20()
{
    Assert(iscntrl(32)  == 0 ,"iscntrl should be 0 for  ");
}


void t_iscntrl_0x21()
{
    Assert(iscntrl(33)  == 0 ,"iscntrl should be 0 for !");
}


void t_iscntrl_0x22()
{
    Assert(iscntrl(34)  == 0 ,"iscntrl should be 0 for 0x22");
}


void t_iscntrl_0x23()
{
    Assert(iscntrl(35)  == 0 ,"iscntrl should be 0 for #");
}


void t_iscntrl_0x24()
{
    Assert(iscntrl(36)  == 0 ,"iscntrl should be 0 for $");
}


void t_iscntrl_0x25()
{
    Assert(iscntrl(37)  == 0 ,"iscntrl should be 0 for %");
}


void t_iscntrl_0x26()
{
    Assert(iscntrl(38)  == 0 ,"iscntrl should be 0 for &");
}


void t_iscntrl_0x27()
{
    Assert(iscntrl(39)  == 0 ,"iscntrl should be 0 for '");
}


void t_iscntrl_0x28()
{
    Assert(iscntrl(40)  == 0 ,"iscntrl should be 0 for (");
}


void t_iscntrl_0x29()
{
    Assert(iscntrl(41)  == 0 ,"iscntrl should be 0 for )");
}


void t_iscntrl_0x2a()
{
    Assert(iscntrl(42)  == 0 ,"iscntrl should be 0 for *");
}


void t_iscntrl_0x2b()
{
    Assert(iscntrl(43)  == 0 ,"iscntrl should be 0 for +");
}


void t_iscntrl_0x2c()
{
    Assert(iscntrl(44)  == 0 ,"iscntrl should be 0 for ,");
}


void t_iscntrl_0x2d()
{
    Assert(iscntrl(45)  == 0 ,"iscntrl should be 0 for -");
}


void t_iscntrl_0x2e()
{
    Assert(iscntrl(46)  == 0 ,"iscntrl should be 0 for .");
}


void t_iscntrl_0x2f()
{
    Assert(iscntrl(47)  == 0 ,"iscntrl should be 0 for /");
}


void t_iscntrl_0x30()
{
    Assert(iscntrl(48)  == 0 ,"iscntrl should be 0 for 0");
}


void t_iscntrl_0x31()
{
    Assert(iscntrl(49)  == 0 ,"iscntrl should be 0 for 1");
}


void t_iscntrl_0x32()
{
    Assert(iscntrl(50)  == 0 ,"iscntrl should be 0 for 2");
}


void t_iscntrl_0x33()
{
    Assert(iscntrl(51)  == 0 ,"iscntrl should be 0 for 3");
}


void t_iscntrl_0x34()
{
    Assert(iscntrl(52)  == 0 ,"iscntrl should be 0 for 4");
}


void t_iscntrl_0x35()
{
    Assert(iscntrl(53)  == 0 ,"iscntrl should be 0 for 5");
}


void t_iscntrl_0x36()
{
    Assert(iscntrl(54)  == 0 ,"iscntrl should be 0 for 6");
}


void t_iscntrl_0x37()
{
    Assert(iscntrl(55)  == 0 ,"iscntrl should be 0 for 7");
}


void t_iscntrl_0x38()
{
    Assert(iscntrl(56)  == 0 ,"iscntrl should be 0 for 8");
}


void t_iscntrl_0x39()
{
    Assert(iscntrl(57)  == 0 ,"iscntrl should be 0 for 9");
}


void t_iscntrl_0x3a()
{
    Assert(iscntrl(58)  == 0 ,"iscntrl should be 0 for :");
}


void t_iscntrl_0x3b()
{
    Assert(iscntrl(59)  == 0 ,"iscntrl should be 0 for ;");
}


void t_iscntrl_0x3c()
{
    Assert(iscntrl(60)  == 0 ,"iscntrl should be 0 for <");
}


void t_iscntrl_0x3d()
{
    Assert(iscntrl(61)  == 0 ,"iscntrl should be 0 for =");
}


void t_iscntrl_0x3e()
{
    Assert(iscntrl(62)  == 0 ,"iscntrl should be 0 for >");
}


void t_iscntrl_0x3f()
{
    Assert(iscntrl(63)  == 0 ,"iscntrl should be 0 for ?");
}


int main(void)
{
    suite_setup("test_iscntrl_shard_01");
    suite_add_test(t_iscntrl_0x20);
    suite_add_test(t_iscntrl_0x21);
    suite_add_test(t_iscntrl_0x22);
    suite_add_test(t_iscntrl_0x23);
    suite_add_test(t_iscntrl_0x24);
    suite_add_test(t_iscntrl_0x25);
    suite_add_test(t_iscntrl_0x26);
    suite_add_test(t_iscntrl_0x27);
    suite_add_test(t_iscntrl_0x28);
    suite_add_test(t_iscntrl_0x29);
    suite_add_test(t_iscntrl_0x2a);
    suite_add_test(t_iscntrl_0x2b);
    suite_add_test(t_iscntrl_0x2c);
    suite_add_test(t_iscntrl_0x2d);
    suite_add_test(t_iscntrl_0x2e);
    suite_add_test(t_iscntrl_0x2f);
    suite_add_test(t_iscntrl_0x30);
    suite_add_test(t_iscntrl_0x31);
    suite_add_test(t_iscntrl_0x32);
    suite_add_test(t_iscntrl_0x33);
    suite_add_test(t_iscntrl_0x34);
    suite_add_test(t_iscntrl_0x35);
    suite_add_test(t_iscntrl_0x36);
    suite_add_test(t_iscntrl_0x37);
    suite_add_test(t_iscntrl_0x38);
    suite_add_test(t_iscntrl_0x39);
    suite_add_test(t_iscntrl_0x3a);
    suite_add_test(t_iscntrl_0x3b);
    suite_add_test(t_iscntrl_0x3c);
    suite_add_test(t_iscntrl_0x3d);
    suite_add_test(t_iscntrl_0x3e);
    suite_add_test(t_iscntrl_0x3f);
    return suite_run();
}
