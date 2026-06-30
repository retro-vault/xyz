/* auto-generated z88dk ctype shard */
#include "../../upstream/test/suites/ctype/ctype_test.h"

void t_islower_0x20()
{
    Assert(islower(32)  == 0 ,"islower should be 0 for  ");
}


void t_islower_0x21()
{
    Assert(islower(33)  == 0 ,"islower should be 0 for !");
}


void t_islower_0x22()
{
    Assert(islower(34)  == 0 ,"islower should be 0 for 0x22");
}


void t_islower_0x23()
{
    Assert(islower(35)  == 0 ,"islower should be 0 for #");
}


void t_islower_0x24()
{
    Assert(islower(36)  == 0 ,"islower should be 0 for $");
}


void t_islower_0x25()
{
    Assert(islower(37)  == 0 ,"islower should be 0 for %");
}


void t_islower_0x26()
{
    Assert(islower(38)  == 0 ,"islower should be 0 for &");
}


void t_islower_0x27()
{
    Assert(islower(39)  == 0 ,"islower should be 0 for '");
}


void t_islower_0x28()
{
    Assert(islower(40)  == 0 ,"islower should be 0 for (");
}


void t_islower_0x29()
{
    Assert(islower(41)  == 0 ,"islower should be 0 for )");
}


void t_islower_0x2a()
{
    Assert(islower(42)  == 0 ,"islower should be 0 for *");
}


void t_islower_0x2b()
{
    Assert(islower(43)  == 0 ,"islower should be 0 for +");
}


void t_islower_0x2c()
{
    Assert(islower(44)  == 0 ,"islower should be 0 for ,");
}


void t_islower_0x2d()
{
    Assert(islower(45)  == 0 ,"islower should be 0 for -");
}


void t_islower_0x2e()
{
    Assert(islower(46)  == 0 ,"islower should be 0 for .");
}


void t_islower_0x2f()
{
    Assert(islower(47)  == 0 ,"islower should be 0 for /");
}


void t_islower_0x30()
{
    Assert(islower(48)  == 0 ,"islower should be 0 for 0");
}


void t_islower_0x31()
{
    Assert(islower(49)  == 0 ,"islower should be 0 for 1");
}


void t_islower_0x32()
{
    Assert(islower(50)  == 0 ,"islower should be 0 for 2");
}


void t_islower_0x33()
{
    Assert(islower(51)  == 0 ,"islower should be 0 for 3");
}


void t_islower_0x34()
{
    Assert(islower(52)  == 0 ,"islower should be 0 for 4");
}


void t_islower_0x35()
{
    Assert(islower(53)  == 0 ,"islower should be 0 for 5");
}


void t_islower_0x36()
{
    Assert(islower(54)  == 0 ,"islower should be 0 for 6");
}


void t_islower_0x37()
{
    Assert(islower(55)  == 0 ,"islower should be 0 for 7");
}


void t_islower_0x38()
{
    Assert(islower(56)  == 0 ,"islower should be 0 for 8");
}


void t_islower_0x39()
{
    Assert(islower(57)  == 0 ,"islower should be 0 for 9");
}


void t_islower_0x3a()
{
    Assert(islower(58)  == 0 ,"islower should be 0 for :");
}


void t_islower_0x3b()
{
    Assert(islower(59)  == 0 ,"islower should be 0 for ;");
}


void t_islower_0x3c()
{
    Assert(islower(60)  == 0 ,"islower should be 0 for <");
}


void t_islower_0x3d()
{
    Assert(islower(61)  == 0 ,"islower should be 0 for =");
}


void t_islower_0x3e()
{
    Assert(islower(62)  == 0 ,"islower should be 0 for >");
}


void t_islower_0x3f()
{
    Assert(islower(63)  == 0 ,"islower should be 0 for ?");
}


int main(void)
{
    suite_setup("test_islower_shard_01");
    suite_add_test(t_islower_0x20);
    suite_add_test(t_islower_0x21);
    suite_add_test(t_islower_0x22);
    suite_add_test(t_islower_0x23);
    suite_add_test(t_islower_0x24);
    suite_add_test(t_islower_0x25);
    suite_add_test(t_islower_0x26);
    suite_add_test(t_islower_0x27);
    suite_add_test(t_islower_0x28);
    suite_add_test(t_islower_0x29);
    suite_add_test(t_islower_0x2a);
    suite_add_test(t_islower_0x2b);
    suite_add_test(t_islower_0x2c);
    suite_add_test(t_islower_0x2d);
    suite_add_test(t_islower_0x2e);
    suite_add_test(t_islower_0x2f);
    suite_add_test(t_islower_0x30);
    suite_add_test(t_islower_0x31);
    suite_add_test(t_islower_0x32);
    suite_add_test(t_islower_0x33);
    suite_add_test(t_islower_0x34);
    suite_add_test(t_islower_0x35);
    suite_add_test(t_islower_0x36);
    suite_add_test(t_islower_0x37);
    suite_add_test(t_islower_0x38);
    suite_add_test(t_islower_0x39);
    suite_add_test(t_islower_0x3a);
    suite_add_test(t_islower_0x3b);
    suite_add_test(t_islower_0x3c);
    suite_add_test(t_islower_0x3d);
    suite_add_test(t_islower_0x3e);
    suite_add_test(t_islower_0x3f);
    return suite_run();
}
