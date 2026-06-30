/* auto-generated z88dk ctype shard */
#include "../../upstream/test/suites/ctype/ctype_test.h"

void t_isalnum_0x60()
{
    Assert(isalnum(96)  == 0 ,"isalnum should be 0 for `");
}


void t_isalnum_0x61()
{
    Assert(isalnum(97) ,"isalnum should be 1 for a");
}


void t_isalnum_0x62()
{
    Assert(isalnum(98) ,"isalnum should be 1 for b");
}


void t_isalnum_0x63()
{
    Assert(isalnum(99) ,"isalnum should be 1 for c");
}


void t_isalnum_0x64()
{
    Assert(isalnum(100) ,"isalnum should be 1 for d");
}


void t_isalnum_0x65()
{
    Assert(isalnum(101) ,"isalnum should be 1 for e");
}


void t_isalnum_0x66()
{
    Assert(isalnum(102) ,"isalnum should be 1 for f");
}


void t_isalnum_0x67()
{
    Assert(isalnum(103) ,"isalnum should be 1 for g");
}


void t_isalnum_0x68()
{
    Assert(isalnum(104) ,"isalnum should be 1 for h");
}


void t_isalnum_0x69()
{
    Assert(isalnum(105) ,"isalnum should be 1 for i");
}


void t_isalnum_0x6a()
{
    Assert(isalnum(106) ,"isalnum should be 1 for j");
}


void t_isalnum_0x6b()
{
    Assert(isalnum(107) ,"isalnum should be 1 for k");
}


void t_isalnum_0x6c()
{
    Assert(isalnum(108) ,"isalnum should be 1 for l");
}


void t_isalnum_0x6d()
{
    Assert(isalnum(109) ,"isalnum should be 1 for m");
}


void t_isalnum_0x6e()
{
    Assert(isalnum(110) ,"isalnum should be 1 for n");
}


void t_isalnum_0x6f()
{
    Assert(isalnum(111) ,"isalnum should be 1 for o");
}


void t_isalnum_0x70()
{
    Assert(isalnum(112) ,"isalnum should be 1 for p");
}


void t_isalnum_0x71()
{
    Assert(isalnum(113) ,"isalnum should be 1 for q");
}


void t_isalnum_0x72()
{
    Assert(isalnum(114) ,"isalnum should be 1 for r");
}


void t_isalnum_0x73()
{
    Assert(isalnum(115) ,"isalnum should be 1 for s");
}


void t_isalnum_0x74()
{
    Assert(isalnum(116) ,"isalnum should be 1 for t");
}


void t_isalnum_0x75()
{
    Assert(isalnum(117) ,"isalnum should be 1 for u");
}


void t_isalnum_0x76()
{
    Assert(isalnum(118) ,"isalnum should be 1 for v");
}


void t_isalnum_0x77()
{
    Assert(isalnum(119) ,"isalnum should be 1 for w");
}


void t_isalnum_0x78()
{
    Assert(isalnum(120) ,"isalnum should be 1 for x");
}


void t_isalnum_0x79()
{
    Assert(isalnum(121) ,"isalnum should be 1 for y");
}


void t_isalnum_0x7a()
{
    Assert(isalnum(122) ,"isalnum should be 1 for z");
}


void t_isalnum_0x7b()
{
    Assert(isalnum(123)  == 0 ,"isalnum should be 0 for {");
}


void t_isalnum_0x7c()
{
    Assert(isalnum(124)  == 0 ,"isalnum should be 0 for |");
}


void t_isalnum_0x7d()
{
    Assert(isalnum(125)  == 0 ,"isalnum should be 0 for }");
}


void t_isalnum_0x7e()
{
    Assert(isalnum(126)  == 0 ,"isalnum should be 0 for ~");
}


void t_isalnum_0x7f()
{
    Assert(isalnum(127)  == 0 ,"isalnum should be 0 for 0x7f");
}


int main(void)
{
    suite_setup("test_isalnum_shard_03");
    suite_add_test(t_isalnum_0x60);
    suite_add_test(t_isalnum_0x61);
    suite_add_test(t_isalnum_0x62);
    suite_add_test(t_isalnum_0x63);
    suite_add_test(t_isalnum_0x64);
    suite_add_test(t_isalnum_0x65);
    suite_add_test(t_isalnum_0x66);
    suite_add_test(t_isalnum_0x67);
    suite_add_test(t_isalnum_0x68);
    suite_add_test(t_isalnum_0x69);
    suite_add_test(t_isalnum_0x6a);
    suite_add_test(t_isalnum_0x6b);
    suite_add_test(t_isalnum_0x6c);
    suite_add_test(t_isalnum_0x6d);
    suite_add_test(t_isalnum_0x6e);
    suite_add_test(t_isalnum_0x6f);
    suite_add_test(t_isalnum_0x70);
    suite_add_test(t_isalnum_0x71);
    suite_add_test(t_isalnum_0x72);
    suite_add_test(t_isalnum_0x73);
    suite_add_test(t_isalnum_0x74);
    suite_add_test(t_isalnum_0x75);
    suite_add_test(t_isalnum_0x76);
    suite_add_test(t_isalnum_0x77);
    suite_add_test(t_isalnum_0x78);
    suite_add_test(t_isalnum_0x79);
    suite_add_test(t_isalnum_0x7a);
    suite_add_test(t_isalnum_0x7b);
    suite_add_test(t_isalnum_0x7c);
    suite_add_test(t_isalnum_0x7d);
    suite_add_test(t_isalnum_0x7e);
    suite_add_test(t_isalnum_0x7f);
    return suite_run();
}
