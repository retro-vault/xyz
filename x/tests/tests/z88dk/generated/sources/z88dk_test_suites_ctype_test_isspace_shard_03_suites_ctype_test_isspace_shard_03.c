/* auto-generated z88dk ctype shard */
#include "../../upstream/test/suites/ctype/ctype_test.h"

void t_isspace_0x60()
{
    Assert(isspace(96)  == 0 ,"isspace should be 0 for `");
}


void t_isspace_0x61()
{
    Assert(isspace(97)  == 0 ,"isspace should be 0 for a");
}


void t_isspace_0x62()
{
    Assert(isspace(98)  == 0 ,"isspace should be 0 for b");
}


void t_isspace_0x63()
{
    Assert(isspace(99)  == 0 ,"isspace should be 0 for c");
}


void t_isspace_0x64()
{
    Assert(isspace(100)  == 0 ,"isspace should be 0 for d");
}


void t_isspace_0x65()
{
    Assert(isspace(101)  == 0 ,"isspace should be 0 for e");
}


void t_isspace_0x66()
{
    Assert(isspace(102)  == 0 ,"isspace should be 0 for f");
}


void t_isspace_0x67()
{
    Assert(isspace(103)  == 0 ,"isspace should be 0 for g");
}


void t_isspace_0x68()
{
    Assert(isspace(104)  == 0 ,"isspace should be 0 for h");
}


void t_isspace_0x69()
{
    Assert(isspace(105)  == 0 ,"isspace should be 0 for i");
}


void t_isspace_0x6a()
{
    Assert(isspace(106)  == 0 ,"isspace should be 0 for j");
}


void t_isspace_0x6b()
{
    Assert(isspace(107)  == 0 ,"isspace should be 0 for k");
}


void t_isspace_0x6c()
{
    Assert(isspace(108)  == 0 ,"isspace should be 0 for l");
}


void t_isspace_0x6d()
{
    Assert(isspace(109)  == 0 ,"isspace should be 0 for m");
}


void t_isspace_0x6e()
{
    Assert(isspace(110)  == 0 ,"isspace should be 0 for n");
}


void t_isspace_0x6f()
{
    Assert(isspace(111)  == 0 ,"isspace should be 0 for o");
}


void t_isspace_0x70()
{
    Assert(isspace(112)  == 0 ,"isspace should be 0 for p");
}


void t_isspace_0x71()
{
    Assert(isspace(113)  == 0 ,"isspace should be 0 for q");
}


void t_isspace_0x72()
{
    Assert(isspace(114)  == 0 ,"isspace should be 0 for r");
}


void t_isspace_0x73()
{
    Assert(isspace(115)  == 0 ,"isspace should be 0 for s");
}


void t_isspace_0x74()
{
    Assert(isspace(116)  == 0 ,"isspace should be 0 for t");
}


void t_isspace_0x75()
{
    Assert(isspace(117)  == 0 ,"isspace should be 0 for u");
}


void t_isspace_0x76()
{
    Assert(isspace(118)  == 0 ,"isspace should be 0 for v");
}


void t_isspace_0x77()
{
    Assert(isspace(119)  == 0 ,"isspace should be 0 for w");
}


void t_isspace_0x78()
{
    Assert(isspace(120)  == 0 ,"isspace should be 0 for x");
}


void t_isspace_0x79()
{
    Assert(isspace(121)  == 0 ,"isspace should be 0 for y");
}


void t_isspace_0x7a()
{
    Assert(isspace(122)  == 0 ,"isspace should be 0 for z");
}


void t_isspace_0x7b()
{
    Assert(isspace(123)  == 0 ,"isspace should be 0 for {");
}


void t_isspace_0x7c()
{
    Assert(isspace(124)  == 0 ,"isspace should be 0 for |");
}


void t_isspace_0x7d()
{
    Assert(isspace(125)  == 0 ,"isspace should be 0 for }");
}


void t_isspace_0x7e()
{
    Assert(isspace(126)  == 0 ,"isspace should be 0 for ~");
}


void t_isspace_0x7f()
{
    Assert(isspace(127)  == 0 ,"isspace should be 0 for 0x7f");
}


int main(void)
{
    suite_setup("test_isspace_shard_03");
    suite_add_test(t_isspace_0x60);
    suite_add_test(t_isspace_0x61);
    suite_add_test(t_isspace_0x62);
    suite_add_test(t_isspace_0x63);
    suite_add_test(t_isspace_0x64);
    suite_add_test(t_isspace_0x65);
    suite_add_test(t_isspace_0x66);
    suite_add_test(t_isspace_0x67);
    suite_add_test(t_isspace_0x68);
    suite_add_test(t_isspace_0x69);
    suite_add_test(t_isspace_0x6a);
    suite_add_test(t_isspace_0x6b);
    suite_add_test(t_isspace_0x6c);
    suite_add_test(t_isspace_0x6d);
    suite_add_test(t_isspace_0x6e);
    suite_add_test(t_isspace_0x6f);
    suite_add_test(t_isspace_0x70);
    suite_add_test(t_isspace_0x71);
    suite_add_test(t_isspace_0x72);
    suite_add_test(t_isspace_0x73);
    suite_add_test(t_isspace_0x74);
    suite_add_test(t_isspace_0x75);
    suite_add_test(t_isspace_0x76);
    suite_add_test(t_isspace_0x77);
    suite_add_test(t_isspace_0x78);
    suite_add_test(t_isspace_0x79);
    suite_add_test(t_isspace_0x7a);
    suite_add_test(t_isspace_0x7b);
    suite_add_test(t_isspace_0x7c);
    suite_add_test(t_isspace_0x7d);
    suite_add_test(t_isspace_0x7e);
    suite_add_test(t_isspace_0x7f);
    return suite_run();
}
