/* auto-generated z88dk ctype shard */
#include "../../upstream/test/suites/ctype/ctype_test.h"

void t_isalpha_0x60()
{
    Assert(isalpha(96)  == 0 ,"isalpha should be 0 for `");
}


void t_isalpha_0x61()
{
    Assert(isalpha(97) ,"isalpha should be 1 for a");
}


void t_isalpha_0x62()
{
    Assert(isalpha(98) ,"isalpha should be 1 for b");
}


void t_isalpha_0x63()
{
    Assert(isalpha(99) ,"isalpha should be 1 for c");
}


void t_isalpha_0x64()
{
    Assert(isalpha(100) ,"isalpha should be 1 for d");
}


void t_isalpha_0x65()
{
    Assert(isalpha(101) ,"isalpha should be 1 for e");
}


void t_isalpha_0x66()
{
    Assert(isalpha(102) ,"isalpha should be 1 for f");
}


void t_isalpha_0x67()
{
    Assert(isalpha(103) ,"isalpha should be 1 for g");
}


void t_isalpha_0x68()
{
    Assert(isalpha(104) ,"isalpha should be 1 for h");
}


void t_isalpha_0x69()
{
    Assert(isalpha(105) ,"isalpha should be 1 for i");
}


void t_isalpha_0x6a()
{
    Assert(isalpha(106) ,"isalpha should be 1 for j");
}


void t_isalpha_0x6b()
{
    Assert(isalpha(107) ,"isalpha should be 1 for k");
}


void t_isalpha_0x6c()
{
    Assert(isalpha(108) ,"isalpha should be 1 for l");
}


void t_isalpha_0x6d()
{
    Assert(isalpha(109) ,"isalpha should be 1 for m");
}


void t_isalpha_0x6e()
{
    Assert(isalpha(110) ,"isalpha should be 1 for n");
}


void t_isalpha_0x6f()
{
    Assert(isalpha(111) ,"isalpha should be 1 for o");
}


void t_isalpha_0x70()
{
    Assert(isalpha(112) ,"isalpha should be 1 for p");
}


void t_isalpha_0x71()
{
    Assert(isalpha(113) ,"isalpha should be 1 for q");
}


void t_isalpha_0x72()
{
    Assert(isalpha(114) ,"isalpha should be 1 for r");
}


void t_isalpha_0x73()
{
    Assert(isalpha(115) ,"isalpha should be 1 for s");
}


void t_isalpha_0x74()
{
    Assert(isalpha(116) ,"isalpha should be 1 for t");
}


void t_isalpha_0x75()
{
    Assert(isalpha(117) ,"isalpha should be 1 for u");
}


void t_isalpha_0x76()
{
    Assert(isalpha(118) ,"isalpha should be 1 for v");
}


void t_isalpha_0x77()
{
    Assert(isalpha(119) ,"isalpha should be 1 for w");
}


void t_isalpha_0x78()
{
    Assert(isalpha(120) ,"isalpha should be 1 for x");
}


void t_isalpha_0x79()
{
    Assert(isalpha(121) ,"isalpha should be 1 for y");
}


void t_isalpha_0x7a()
{
    Assert(isalpha(122) ,"isalpha should be 1 for z");
}


void t_isalpha_0x7b()
{
    Assert(isalpha(123)  == 0 ,"isalpha should be 0 for {");
}


void t_isalpha_0x7c()
{
    Assert(isalpha(124)  == 0 ,"isalpha should be 0 for |");
}


void t_isalpha_0x7d()
{
    Assert(isalpha(125)  == 0 ,"isalpha should be 0 for }");
}


void t_isalpha_0x7e()
{
    Assert(isalpha(126)  == 0 ,"isalpha should be 0 for ~");
}


void t_isalpha_0x7f()
{
    Assert(isalpha(127)  == 0 ,"isalpha should be 0 for 0x7f");
}


int main(void)
{
    suite_setup("test_isalpha_shard_03");
    suite_add_test(t_isalpha_0x60);
    suite_add_test(t_isalpha_0x61);
    suite_add_test(t_isalpha_0x62);
    suite_add_test(t_isalpha_0x63);
    suite_add_test(t_isalpha_0x64);
    suite_add_test(t_isalpha_0x65);
    suite_add_test(t_isalpha_0x66);
    suite_add_test(t_isalpha_0x67);
    suite_add_test(t_isalpha_0x68);
    suite_add_test(t_isalpha_0x69);
    suite_add_test(t_isalpha_0x6a);
    suite_add_test(t_isalpha_0x6b);
    suite_add_test(t_isalpha_0x6c);
    suite_add_test(t_isalpha_0x6d);
    suite_add_test(t_isalpha_0x6e);
    suite_add_test(t_isalpha_0x6f);
    suite_add_test(t_isalpha_0x70);
    suite_add_test(t_isalpha_0x71);
    suite_add_test(t_isalpha_0x72);
    suite_add_test(t_isalpha_0x73);
    suite_add_test(t_isalpha_0x74);
    suite_add_test(t_isalpha_0x75);
    suite_add_test(t_isalpha_0x76);
    suite_add_test(t_isalpha_0x77);
    suite_add_test(t_isalpha_0x78);
    suite_add_test(t_isalpha_0x79);
    suite_add_test(t_isalpha_0x7a);
    suite_add_test(t_isalpha_0x7b);
    suite_add_test(t_isalpha_0x7c);
    suite_add_test(t_isalpha_0x7d);
    suite_add_test(t_isalpha_0x7e);
    suite_add_test(t_isalpha_0x7f);
    return suite_run();
}
