/* auto-generated z88dk ctype shard */
#include "../../upstream/test/suites/ctype/ctype_test.h"

void t_isascii_0x00()
{
    Assert(isascii(0) ,"isascii should be 1 for 0x00");
}


void t_isascii_0x01()
{
    Assert(isascii(1) ,"isascii should be 1 for 0x01");
}


void t_isascii_0x02()
{
    Assert(isascii(2) ,"isascii should be 1 for 0x02");
}


void t_isascii_0x03()
{
    Assert(isascii(3) ,"isascii should be 1 for 0x03");
}


void t_isascii_0x04()
{
    Assert(isascii(4) ,"isascii should be 1 for 0x04");
}


void t_isascii_0x05()
{
    Assert(isascii(5) ,"isascii should be 1 for 0x05");
}


void t_isascii_0x06()
{
    Assert(isascii(6) ,"isascii should be 1 for 0x06");
}


void t_isascii_0x07()
{
    Assert(isascii(7) ,"isascii should be 1 for 0x07");
}


void t_isascii_0x08()
{
    Assert(isascii(8) ,"isascii should be 1 for 0x08");
}


void t_isascii_0x09()
{
    Assert(isascii(9) ,"isascii should be 1 for 0x09");
}


void t_isascii_0x0a()
{
    Assert(isascii(10) ,"isascii should be 1 for 0x0a");
}


void t_isascii_0x0b()
{
    Assert(isascii(11) ,"isascii should be 1 for 0x0b");
}


void t_isascii_0x0c()
{
    Assert(isascii(12) ,"isascii should be 1 for 0x0c");
}


void t_isascii_0x0d()
{
    Assert(isascii(13) ,"isascii should be 1 for 0x0d");
}


void t_isascii_0x0e()
{
    Assert(isascii(14) ,"isascii should be 1 for 0x0e");
}


void t_isascii_0x0f()
{
    Assert(isascii(15) ,"isascii should be 1 for 0x0f");
}


void t_isascii_0x10()
{
    Assert(isascii(16) ,"isascii should be 1 for 0x10");
}


void t_isascii_0x11()
{
    Assert(isascii(17) ,"isascii should be 1 for 0x11");
}


void t_isascii_0x12()
{
    Assert(isascii(18) ,"isascii should be 1 for 0x12");
}


void t_isascii_0x13()
{
    Assert(isascii(19) ,"isascii should be 1 for 0x13");
}


void t_isascii_0x14()
{
    Assert(isascii(20) ,"isascii should be 1 for 0x14");
}


void t_isascii_0x15()
{
    Assert(isascii(21) ,"isascii should be 1 for 0x15");
}


void t_isascii_0x16()
{
    Assert(isascii(22) ,"isascii should be 1 for 0x16");
}


void t_isascii_0x17()
{
    Assert(isascii(23) ,"isascii should be 1 for 0x17");
}


void t_isascii_0x18()
{
    Assert(isascii(24) ,"isascii should be 1 for 0x18");
}


void t_isascii_0x19()
{
    Assert(isascii(25) ,"isascii should be 1 for 0x19");
}


void t_isascii_0x1a()
{
    Assert(isascii(26) ,"isascii should be 1 for 0x1a");
}


void t_isascii_0x1b()
{
    Assert(isascii(27) ,"isascii should be 1 for 0x1b");
}


void t_isascii_0x1c()
{
    Assert(isascii(28) ,"isascii should be 1 for 0x1c");
}


void t_isascii_0x1d()
{
    Assert(isascii(29) ,"isascii should be 1 for 0x1d");
}


void t_isascii_0x1e()
{
    Assert(isascii(30) ,"isascii should be 1 for 0x1e");
}


void t_isascii_0x1f()
{
    Assert(isascii(31) ,"isascii should be 1 for 0x1f");
}


int main(void)
{
    suite_setup("test_isascii_shard_00");
    suite_add_test(t_isascii_0x00);
    suite_add_test(t_isascii_0x01);
    suite_add_test(t_isascii_0x02);
    suite_add_test(t_isascii_0x03);
    suite_add_test(t_isascii_0x04);
    suite_add_test(t_isascii_0x05);
    suite_add_test(t_isascii_0x06);
    suite_add_test(t_isascii_0x07);
    suite_add_test(t_isascii_0x08);
    suite_add_test(t_isascii_0x09);
    suite_add_test(t_isascii_0x0a);
    suite_add_test(t_isascii_0x0b);
    suite_add_test(t_isascii_0x0c);
    suite_add_test(t_isascii_0x0d);
    suite_add_test(t_isascii_0x0e);
    suite_add_test(t_isascii_0x0f);
    suite_add_test(t_isascii_0x10);
    suite_add_test(t_isascii_0x11);
    suite_add_test(t_isascii_0x12);
    suite_add_test(t_isascii_0x13);
    suite_add_test(t_isascii_0x14);
    suite_add_test(t_isascii_0x15);
    suite_add_test(t_isascii_0x16);
    suite_add_test(t_isascii_0x17);
    suite_add_test(t_isascii_0x18);
    suite_add_test(t_isascii_0x19);
    suite_add_test(t_isascii_0x1a);
    suite_add_test(t_isascii_0x1b);
    suite_add_test(t_isascii_0x1c);
    suite_add_test(t_isascii_0x1d);
    suite_add_test(t_isascii_0x1e);
    suite_add_test(t_isascii_0x1f);
    return suite_run();
}
