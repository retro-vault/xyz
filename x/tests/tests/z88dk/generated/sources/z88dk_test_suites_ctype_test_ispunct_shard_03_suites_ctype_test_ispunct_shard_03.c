/* auto-generated z88dk ctype shard */
#include "../../upstream/test/suites/ctype/ctype_test.h"

void t_ispunct_0x60()
{
    Assert(ispunct(96) ,"ispunct should be 1 for `");
}


void t_ispunct_0x61()
{
    Assert(ispunct(97)  == 0 ,"ispunct should be 0 for a");
}


void t_ispunct_0x62()
{
    Assert(ispunct(98)  == 0 ,"ispunct should be 0 for b");
}


void t_ispunct_0x63()
{
    Assert(ispunct(99)  == 0 ,"ispunct should be 0 for c");
}


void t_ispunct_0x64()
{
    Assert(ispunct(100)  == 0 ,"ispunct should be 0 for d");
}


void t_ispunct_0x65()
{
    Assert(ispunct(101)  == 0 ,"ispunct should be 0 for e");
}


void t_ispunct_0x66()
{
    Assert(ispunct(102)  == 0 ,"ispunct should be 0 for f");
}


void t_ispunct_0x67()
{
    Assert(ispunct(103)  == 0 ,"ispunct should be 0 for g");
}


void t_ispunct_0x68()
{
    Assert(ispunct(104)  == 0 ,"ispunct should be 0 for h");
}


void t_ispunct_0x69()
{
    Assert(ispunct(105)  == 0 ,"ispunct should be 0 for i");
}


void t_ispunct_0x6a()
{
    Assert(ispunct(106)  == 0 ,"ispunct should be 0 for j");
}


void t_ispunct_0x6b()
{
    Assert(ispunct(107)  == 0 ,"ispunct should be 0 for k");
}


void t_ispunct_0x6c()
{
    Assert(ispunct(108)  == 0 ,"ispunct should be 0 for l");
}


void t_ispunct_0x6d()
{
    Assert(ispunct(109)  == 0 ,"ispunct should be 0 for m");
}


void t_ispunct_0x6e()
{
    Assert(ispunct(110)  == 0 ,"ispunct should be 0 for n");
}


void t_ispunct_0x6f()
{
    Assert(ispunct(111)  == 0 ,"ispunct should be 0 for o");
}


void t_ispunct_0x70()
{
    Assert(ispunct(112)  == 0 ,"ispunct should be 0 for p");
}


void t_ispunct_0x71()
{
    Assert(ispunct(113)  == 0 ,"ispunct should be 0 for q");
}


void t_ispunct_0x72()
{
    Assert(ispunct(114)  == 0 ,"ispunct should be 0 for r");
}


void t_ispunct_0x73()
{
    Assert(ispunct(115)  == 0 ,"ispunct should be 0 for s");
}


void t_ispunct_0x74()
{
    Assert(ispunct(116)  == 0 ,"ispunct should be 0 for t");
}


void t_ispunct_0x75()
{
    Assert(ispunct(117)  == 0 ,"ispunct should be 0 for u");
}


void t_ispunct_0x76()
{
    Assert(ispunct(118)  == 0 ,"ispunct should be 0 for v");
}


void t_ispunct_0x77()
{
    Assert(ispunct(119)  == 0 ,"ispunct should be 0 for w");
}


void t_ispunct_0x78()
{
    Assert(ispunct(120)  == 0 ,"ispunct should be 0 for x");
}


void t_ispunct_0x79()
{
    Assert(ispunct(121)  == 0 ,"ispunct should be 0 for y");
}


void t_ispunct_0x7a()
{
    Assert(ispunct(122)  == 0 ,"ispunct should be 0 for z");
}


void t_ispunct_0x7b()
{
    Assert(ispunct(123) ,"ispunct should be 1 for {");
}


void t_ispunct_0x7c()
{
    Assert(ispunct(124) ,"ispunct should be 1 for |");
}


void t_ispunct_0x7d()
{
    Assert(ispunct(125) ,"ispunct should be 1 for }");
}


void t_ispunct_0x7e()
{
    Assert(ispunct(126) ,"ispunct should be 1 for ~");
}


void t_ispunct_0x7f()
{
    Assert(ispunct(127)  == 0 ,"ispunct should be 0 for 0x7f");
}


int main(void)
{
    suite_setup("test_ispunct_shard_03");
    suite_add_test(t_ispunct_0x60);
    suite_add_test(t_ispunct_0x61);
    suite_add_test(t_ispunct_0x62);
    suite_add_test(t_ispunct_0x63);
    suite_add_test(t_ispunct_0x64);
    suite_add_test(t_ispunct_0x65);
    suite_add_test(t_ispunct_0x66);
    suite_add_test(t_ispunct_0x67);
    suite_add_test(t_ispunct_0x68);
    suite_add_test(t_ispunct_0x69);
    suite_add_test(t_ispunct_0x6a);
    suite_add_test(t_ispunct_0x6b);
    suite_add_test(t_ispunct_0x6c);
    suite_add_test(t_ispunct_0x6d);
    suite_add_test(t_ispunct_0x6e);
    suite_add_test(t_ispunct_0x6f);
    suite_add_test(t_ispunct_0x70);
    suite_add_test(t_ispunct_0x71);
    suite_add_test(t_ispunct_0x72);
    suite_add_test(t_ispunct_0x73);
    suite_add_test(t_ispunct_0x74);
    suite_add_test(t_ispunct_0x75);
    suite_add_test(t_ispunct_0x76);
    suite_add_test(t_ispunct_0x77);
    suite_add_test(t_ispunct_0x78);
    suite_add_test(t_ispunct_0x79);
    suite_add_test(t_ispunct_0x7a);
    suite_add_test(t_ispunct_0x7b);
    suite_add_test(t_ispunct_0x7c);
    suite_add_test(t_ispunct_0x7d);
    suite_add_test(t_ispunct_0x7e);
    suite_add_test(t_ispunct_0x7f);
    return suite_run();
}
