/* auto-generated z88dk ctype shard */
#include "../../upstream/test/suites/ctype/ctype_test.h"

void t_isupper_0x20()
{
    Assert(isupper(32)  == 0 ,"isupper should be 0 for  ");
}


void t_isupper_0x21()
{
    Assert(isupper(33)  == 0 ,"isupper should be 0 for !");
}


void t_isupper_0x22()
{
    Assert(isupper(34)  == 0 ,"isupper should be 0 for 0x22");
}


void t_isupper_0x23()
{
    Assert(isupper(35)  == 0 ,"isupper should be 0 for #");
}


void t_isupper_0x24()
{
    Assert(isupper(36)  == 0 ,"isupper should be 0 for $");
}


void t_isupper_0x25()
{
    Assert(isupper(37)  == 0 ,"isupper should be 0 for %");
}


void t_isupper_0x26()
{
    Assert(isupper(38)  == 0 ,"isupper should be 0 for &");
}


void t_isupper_0x27()
{
    Assert(isupper(39)  == 0 ,"isupper should be 0 for '");
}


void t_isupper_0x28()
{
    Assert(isupper(40)  == 0 ,"isupper should be 0 for (");
}


void t_isupper_0x29()
{
    Assert(isupper(41)  == 0 ,"isupper should be 0 for )");
}


void t_isupper_0x2a()
{
    Assert(isupper(42)  == 0 ,"isupper should be 0 for *");
}


void t_isupper_0x2b()
{
    Assert(isupper(43)  == 0 ,"isupper should be 0 for +");
}


void t_isupper_0x2c()
{
    Assert(isupper(44)  == 0 ,"isupper should be 0 for ,");
}


void t_isupper_0x2d()
{
    Assert(isupper(45)  == 0 ,"isupper should be 0 for -");
}


void t_isupper_0x2e()
{
    Assert(isupper(46)  == 0 ,"isupper should be 0 for .");
}


void t_isupper_0x2f()
{
    Assert(isupper(47)  == 0 ,"isupper should be 0 for /");
}


void t_isupper_0x30()
{
    Assert(isupper(48)  == 0 ,"isupper should be 0 for 0");
}


void t_isupper_0x31()
{
    Assert(isupper(49)  == 0 ,"isupper should be 0 for 1");
}


void t_isupper_0x32()
{
    Assert(isupper(50)  == 0 ,"isupper should be 0 for 2");
}


void t_isupper_0x33()
{
    Assert(isupper(51)  == 0 ,"isupper should be 0 for 3");
}


void t_isupper_0x34()
{
    Assert(isupper(52)  == 0 ,"isupper should be 0 for 4");
}


void t_isupper_0x35()
{
    Assert(isupper(53)  == 0 ,"isupper should be 0 for 5");
}


void t_isupper_0x36()
{
    Assert(isupper(54)  == 0 ,"isupper should be 0 for 6");
}


void t_isupper_0x37()
{
    Assert(isupper(55)  == 0 ,"isupper should be 0 for 7");
}


void t_isupper_0x38()
{
    Assert(isupper(56)  == 0 ,"isupper should be 0 for 8");
}


void t_isupper_0x39()
{
    Assert(isupper(57)  == 0 ,"isupper should be 0 for 9");
}


void t_isupper_0x3a()
{
    Assert(isupper(58)  == 0 ,"isupper should be 0 for :");
}


void t_isupper_0x3b()
{
    Assert(isupper(59)  == 0 ,"isupper should be 0 for ;");
}


void t_isupper_0x3c()
{
    Assert(isupper(60)  == 0 ,"isupper should be 0 for <");
}


void t_isupper_0x3d()
{
    Assert(isupper(61)  == 0 ,"isupper should be 0 for =");
}


void t_isupper_0x3e()
{
    Assert(isupper(62)  == 0 ,"isupper should be 0 for >");
}


void t_isupper_0x3f()
{
    Assert(isupper(63)  == 0 ,"isupper should be 0 for ?");
}


int main(void)
{
    suite_setup("test_isupper_shard_01");
    suite_add_test(t_isupper_0x20);
    suite_add_test(t_isupper_0x21);
    suite_add_test(t_isupper_0x22);
    suite_add_test(t_isupper_0x23);
    suite_add_test(t_isupper_0x24);
    suite_add_test(t_isupper_0x25);
    suite_add_test(t_isupper_0x26);
    suite_add_test(t_isupper_0x27);
    suite_add_test(t_isupper_0x28);
    suite_add_test(t_isupper_0x29);
    suite_add_test(t_isupper_0x2a);
    suite_add_test(t_isupper_0x2b);
    suite_add_test(t_isupper_0x2c);
    suite_add_test(t_isupper_0x2d);
    suite_add_test(t_isupper_0x2e);
    suite_add_test(t_isupper_0x2f);
    suite_add_test(t_isupper_0x30);
    suite_add_test(t_isupper_0x31);
    suite_add_test(t_isupper_0x32);
    suite_add_test(t_isupper_0x33);
    suite_add_test(t_isupper_0x34);
    suite_add_test(t_isupper_0x35);
    suite_add_test(t_isupper_0x36);
    suite_add_test(t_isupper_0x37);
    suite_add_test(t_isupper_0x38);
    suite_add_test(t_isupper_0x39);
    suite_add_test(t_isupper_0x3a);
    suite_add_test(t_isupper_0x3b);
    suite_add_test(t_isupper_0x3c);
    suite_add_test(t_isupper_0x3d);
    suite_add_test(t_isupper_0x3e);
    suite_add_test(t_isupper_0x3f);
    return suite_run();
}
