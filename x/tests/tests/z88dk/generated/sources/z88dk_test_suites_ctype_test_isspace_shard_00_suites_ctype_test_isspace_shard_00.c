/* auto-generated z88dk ctype shard */
#include "../../upstream/test/suites/ctype/ctype_test.h"

void t_isspace_0x00()
{
    Assert(isspace(0)  == 0 ,"isspace should be 0 for 0x00");
}


void t_isspace_0x01()
{
    Assert(isspace(1)  == 0 ,"isspace should be 0 for 0x01");
}


void t_isspace_0x02()
{
    Assert(isspace(2)  == 0 ,"isspace should be 0 for 0x02");
}


void t_isspace_0x03()
{
    Assert(isspace(3)  == 0 ,"isspace should be 0 for 0x03");
}


void t_isspace_0x04()
{
    Assert(isspace(4)  == 0 ,"isspace should be 0 for 0x04");
}


void t_isspace_0x05()
{
    Assert(isspace(5)  == 0 ,"isspace should be 0 for 0x05");
}


void t_isspace_0x06()
{
    Assert(isspace(6)  == 0 ,"isspace should be 0 for 0x06");
}


void t_isspace_0x07()
{
    Assert(isspace(7)  == 0 ,"isspace should be 0 for 0x07");
}


void t_isspace_0x08()
{
    Assert(isspace(8)  == 0 ,"isspace should be 0 for 0x08");
}


void t_isspace_0x09()
{
    Assert(isspace(9) ,"isspace should be 1 for 0x09");
}


void t_isspace_0x0a()
{
    Assert(isspace(10) ,"isspace should be 1 for 0x0a");
}


void t_isspace_0x0b()
{
    Assert(isspace(11) ,"isspace should be 1 for 0x0b");
}


void t_isspace_0x0c()
{
    Assert(isspace(12) ,"isspace should be 1 for 0x0c");
}


void t_isspace_0x0d()
{
    Assert(isspace(13) ,"isspace should be 1 for 0x0d");
}


void t_isspace_0x0e()
{
    Assert(isspace(14)  == 0 ,"isspace should be 0 for 0x0e");
}


void t_isspace_0x0f()
{
    Assert(isspace(15)  == 0 ,"isspace should be 0 for 0x0f");
}


void t_isspace_0x10()
{
    Assert(isspace(16)  == 0 ,"isspace should be 0 for 0x10");
}


void t_isspace_0x11()
{
    Assert(isspace(17)  == 0 ,"isspace should be 0 for 0x11");
}


void t_isspace_0x12()
{
    Assert(isspace(18)  == 0 ,"isspace should be 0 for 0x12");
}


void t_isspace_0x13()
{
    Assert(isspace(19)  == 0 ,"isspace should be 0 for 0x13");
}


void t_isspace_0x14()
{
    Assert(isspace(20)  == 0 ,"isspace should be 0 for 0x14");
}


void t_isspace_0x15()
{
    Assert(isspace(21)  == 0 ,"isspace should be 0 for 0x15");
}


void t_isspace_0x16()
{
    Assert(isspace(22)  == 0 ,"isspace should be 0 for 0x16");
}


void t_isspace_0x17()
{
    Assert(isspace(23)  == 0 ,"isspace should be 0 for 0x17");
}


void t_isspace_0x18()
{
    Assert(isspace(24)  == 0 ,"isspace should be 0 for 0x18");
}


void t_isspace_0x19()
{
    Assert(isspace(25)  == 0 ,"isspace should be 0 for 0x19");
}


void t_isspace_0x1a()
{
    Assert(isspace(26)  == 0 ,"isspace should be 0 for 0x1a");
}


void t_isspace_0x1b()
{
    Assert(isspace(27)  == 0 ,"isspace should be 0 for 0x1b");
}


void t_isspace_0x1c()
{
    Assert(isspace(28)  == 0 ,"isspace should be 0 for 0x1c");
}


void t_isspace_0x1d()
{
    Assert(isspace(29)  == 0 ,"isspace should be 0 for 0x1d");
}


void t_isspace_0x1e()
{
    Assert(isspace(30)  == 0 ,"isspace should be 0 for 0x1e");
}


void t_isspace_0x1f()
{
    Assert(isspace(31)  == 0 ,"isspace should be 0 for 0x1f");
}


int main(void)
{
    suite_setup("test_isspace_shard_00");
    suite_add_test(t_isspace_0x00);
    suite_add_test(t_isspace_0x01);
    suite_add_test(t_isspace_0x02);
    suite_add_test(t_isspace_0x03);
    suite_add_test(t_isspace_0x04);
    suite_add_test(t_isspace_0x05);
    suite_add_test(t_isspace_0x06);
    suite_add_test(t_isspace_0x07);
    suite_add_test(t_isspace_0x08);
    suite_add_test(t_isspace_0x09);
    suite_add_test(t_isspace_0x0a);
    suite_add_test(t_isspace_0x0b);
    suite_add_test(t_isspace_0x0c);
    suite_add_test(t_isspace_0x0d);
    suite_add_test(t_isspace_0x0e);
    suite_add_test(t_isspace_0x0f);
    suite_add_test(t_isspace_0x10);
    suite_add_test(t_isspace_0x11);
    suite_add_test(t_isspace_0x12);
    suite_add_test(t_isspace_0x13);
    suite_add_test(t_isspace_0x14);
    suite_add_test(t_isspace_0x15);
    suite_add_test(t_isspace_0x16);
    suite_add_test(t_isspace_0x17);
    suite_add_test(t_isspace_0x18);
    suite_add_test(t_isspace_0x19);
    suite_add_test(t_isspace_0x1a);
    suite_add_test(t_isspace_0x1b);
    suite_add_test(t_isspace_0x1c);
    suite_add_test(t_isspace_0x1d);
    suite_add_test(t_isspace_0x1e);
    suite_add_test(t_isspace_0x1f);
    return suite_run();
}
