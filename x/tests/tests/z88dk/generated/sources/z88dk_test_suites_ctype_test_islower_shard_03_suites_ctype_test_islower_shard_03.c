/* auto-generated z88dk ctype shard */
#include "../../upstream/test/suites/ctype/ctype_test.h"

void t_islower_0x60()
{
    Assert(islower(96)  == 0 ,"islower should be 0 for `");
}


void t_islower_0x61()
{
    Assert(islower(97) ,"islower should be 1 for a");
}


void t_islower_0x62()
{
    Assert(islower(98) ,"islower should be 1 for b");
}


void t_islower_0x63()
{
    Assert(islower(99) ,"islower should be 1 for c");
}


void t_islower_0x64()
{
    Assert(islower(100) ,"islower should be 1 for d");
}


void t_islower_0x65()
{
    Assert(islower(101) ,"islower should be 1 for e");
}


void t_islower_0x66()
{
    Assert(islower(102) ,"islower should be 1 for f");
}


void t_islower_0x67()
{
    Assert(islower(103) ,"islower should be 1 for g");
}


void t_islower_0x68()
{
    Assert(islower(104) ,"islower should be 1 for h");
}


void t_islower_0x69()
{
    Assert(islower(105) ,"islower should be 1 for i");
}


void t_islower_0x6a()
{
    Assert(islower(106) ,"islower should be 1 for j");
}


void t_islower_0x6b()
{
    Assert(islower(107) ,"islower should be 1 for k");
}


void t_islower_0x6c()
{
    Assert(islower(108) ,"islower should be 1 for l");
}


void t_islower_0x6d()
{
    Assert(islower(109) ,"islower should be 1 for m");
}


void t_islower_0x6e()
{
    Assert(islower(110) ,"islower should be 1 for n");
}


void t_islower_0x6f()
{
    Assert(islower(111) ,"islower should be 1 for o");
}


void t_islower_0x70()
{
    Assert(islower(112) ,"islower should be 1 for p");
}


void t_islower_0x71()
{
    Assert(islower(113) ,"islower should be 1 for q");
}


void t_islower_0x72()
{
    Assert(islower(114) ,"islower should be 1 for r");
}


void t_islower_0x73()
{
    Assert(islower(115) ,"islower should be 1 for s");
}


void t_islower_0x74()
{
    Assert(islower(116) ,"islower should be 1 for t");
}


void t_islower_0x75()
{
    Assert(islower(117) ,"islower should be 1 for u");
}


void t_islower_0x76()
{
    Assert(islower(118) ,"islower should be 1 for v");
}


void t_islower_0x77()
{
    Assert(islower(119) ,"islower should be 1 for w");
}


void t_islower_0x78()
{
    Assert(islower(120) ,"islower should be 1 for x");
}


void t_islower_0x79()
{
    Assert(islower(121) ,"islower should be 1 for y");
}


void t_islower_0x7a()
{
    Assert(islower(122) ,"islower should be 1 for z");
}


void t_islower_0x7b()
{
    Assert(islower(123)  == 0 ,"islower should be 0 for {");
}


void t_islower_0x7c()
{
    Assert(islower(124)  == 0 ,"islower should be 0 for |");
}


void t_islower_0x7d()
{
    Assert(islower(125)  == 0 ,"islower should be 0 for }");
}


void t_islower_0x7e()
{
    Assert(islower(126)  == 0 ,"islower should be 0 for ~");
}


void t_islower_0x7f()
{
    Assert(islower(127)  == 0 ,"islower should be 0 for 0x7f");
}


int main(void)
{
    suite_setup("test_islower_shard_03");
    suite_add_test(t_islower_0x60);
    suite_add_test(t_islower_0x61);
    suite_add_test(t_islower_0x62);
    suite_add_test(t_islower_0x63);
    suite_add_test(t_islower_0x64);
    suite_add_test(t_islower_0x65);
    suite_add_test(t_islower_0x66);
    suite_add_test(t_islower_0x67);
    suite_add_test(t_islower_0x68);
    suite_add_test(t_islower_0x69);
    suite_add_test(t_islower_0x6a);
    suite_add_test(t_islower_0x6b);
    suite_add_test(t_islower_0x6c);
    suite_add_test(t_islower_0x6d);
    suite_add_test(t_islower_0x6e);
    suite_add_test(t_islower_0x6f);
    suite_add_test(t_islower_0x70);
    suite_add_test(t_islower_0x71);
    suite_add_test(t_islower_0x72);
    suite_add_test(t_islower_0x73);
    suite_add_test(t_islower_0x74);
    suite_add_test(t_islower_0x75);
    suite_add_test(t_islower_0x76);
    suite_add_test(t_islower_0x77);
    suite_add_test(t_islower_0x78);
    suite_add_test(t_islower_0x79);
    suite_add_test(t_islower_0x7a);
    suite_add_test(t_islower_0x7b);
    suite_add_test(t_islower_0x7c);
    suite_add_test(t_islower_0x7d);
    suite_add_test(t_islower_0x7e);
    suite_add_test(t_islower_0x7f);
    return suite_run();
}
