/* auto-generated z88dk ctype shard */
#include "../../upstream/test/suites/ctype/ctype_test.h"

void t_isgraph_0x60()
{
    Assert(isgraph(96) ,"isgraph should be 1 for `");
}


void t_isgraph_0x61()
{
    Assert(isgraph(97) ,"isgraph should be 1 for a");
}


void t_isgraph_0x62()
{
    Assert(isgraph(98) ,"isgraph should be 1 for b");
}


void t_isgraph_0x63()
{
    Assert(isgraph(99) ,"isgraph should be 1 for c");
}


void t_isgraph_0x64()
{
    Assert(isgraph(100) ,"isgraph should be 1 for d");
}


void t_isgraph_0x65()
{
    Assert(isgraph(101) ,"isgraph should be 1 for e");
}


void t_isgraph_0x66()
{
    Assert(isgraph(102) ,"isgraph should be 1 for f");
}


void t_isgraph_0x67()
{
    Assert(isgraph(103) ,"isgraph should be 1 for g");
}


void t_isgraph_0x68()
{
    Assert(isgraph(104) ,"isgraph should be 1 for h");
}


void t_isgraph_0x69()
{
    Assert(isgraph(105) ,"isgraph should be 1 for i");
}


void t_isgraph_0x6a()
{
    Assert(isgraph(106) ,"isgraph should be 1 for j");
}


void t_isgraph_0x6b()
{
    Assert(isgraph(107) ,"isgraph should be 1 for k");
}


void t_isgraph_0x6c()
{
    Assert(isgraph(108) ,"isgraph should be 1 for l");
}


void t_isgraph_0x6d()
{
    Assert(isgraph(109) ,"isgraph should be 1 for m");
}


void t_isgraph_0x6e()
{
    Assert(isgraph(110) ,"isgraph should be 1 for n");
}


void t_isgraph_0x6f()
{
    Assert(isgraph(111) ,"isgraph should be 1 for o");
}


void t_isgraph_0x70()
{
    Assert(isgraph(112) ,"isgraph should be 1 for p");
}


void t_isgraph_0x71()
{
    Assert(isgraph(113) ,"isgraph should be 1 for q");
}


void t_isgraph_0x72()
{
    Assert(isgraph(114) ,"isgraph should be 1 for r");
}


void t_isgraph_0x73()
{
    Assert(isgraph(115) ,"isgraph should be 1 for s");
}


void t_isgraph_0x74()
{
    Assert(isgraph(116) ,"isgraph should be 1 for t");
}


void t_isgraph_0x75()
{
    Assert(isgraph(117) ,"isgraph should be 1 for u");
}


void t_isgraph_0x76()
{
    Assert(isgraph(118) ,"isgraph should be 1 for v");
}


void t_isgraph_0x77()
{
    Assert(isgraph(119) ,"isgraph should be 1 for w");
}


void t_isgraph_0x78()
{
    Assert(isgraph(120) ,"isgraph should be 1 for x");
}


void t_isgraph_0x79()
{
    Assert(isgraph(121) ,"isgraph should be 1 for y");
}


void t_isgraph_0x7a()
{
    Assert(isgraph(122) ,"isgraph should be 1 for z");
}


void t_isgraph_0x7b()
{
    Assert(isgraph(123) ,"isgraph should be 1 for {");
}


void t_isgraph_0x7c()
{
    Assert(isgraph(124) ,"isgraph should be 1 for |");
}


void t_isgraph_0x7d()
{
    Assert(isgraph(125) ,"isgraph should be 1 for }");
}


void t_isgraph_0x7e()
{
    Assert(isgraph(126) ,"isgraph should be 1 for ~");
}


void t_isgraph_0x7f()
{
    Assert(isgraph(127)  == 0 ,"isgraph should be 0 for 0x7f");
}


int main(void)
{
    suite_setup("test_isgraph_shard_03");
    suite_add_test(t_isgraph_0x60);
    suite_add_test(t_isgraph_0x61);
    suite_add_test(t_isgraph_0x62);
    suite_add_test(t_isgraph_0x63);
    suite_add_test(t_isgraph_0x64);
    suite_add_test(t_isgraph_0x65);
    suite_add_test(t_isgraph_0x66);
    suite_add_test(t_isgraph_0x67);
    suite_add_test(t_isgraph_0x68);
    suite_add_test(t_isgraph_0x69);
    suite_add_test(t_isgraph_0x6a);
    suite_add_test(t_isgraph_0x6b);
    suite_add_test(t_isgraph_0x6c);
    suite_add_test(t_isgraph_0x6d);
    suite_add_test(t_isgraph_0x6e);
    suite_add_test(t_isgraph_0x6f);
    suite_add_test(t_isgraph_0x70);
    suite_add_test(t_isgraph_0x71);
    suite_add_test(t_isgraph_0x72);
    suite_add_test(t_isgraph_0x73);
    suite_add_test(t_isgraph_0x74);
    suite_add_test(t_isgraph_0x75);
    suite_add_test(t_isgraph_0x76);
    suite_add_test(t_isgraph_0x77);
    suite_add_test(t_isgraph_0x78);
    suite_add_test(t_isgraph_0x79);
    suite_add_test(t_isgraph_0x7a);
    suite_add_test(t_isgraph_0x7b);
    suite_add_test(t_isgraph_0x7c);
    suite_add_test(t_isgraph_0x7d);
    suite_add_test(t_isgraph_0x7e);
    suite_add_test(t_isgraph_0x7f);
    return suite_run();
}
