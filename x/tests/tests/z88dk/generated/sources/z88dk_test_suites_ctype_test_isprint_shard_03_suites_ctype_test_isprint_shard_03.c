/* auto-generated z88dk ctype shard */
#include "../../upstream/test/suites/ctype/ctype_test.h"

void t_isprint_0x60()
{
    Assert(isprint(96) ,"isprint should be 1 for `");
}


void t_isprint_0x61()
{
    Assert(isprint(97) ,"isprint should be 1 for a");
}


void t_isprint_0x62()
{
    Assert(isprint(98) ,"isprint should be 1 for b");
}


void t_isprint_0x63()
{
    Assert(isprint(99) ,"isprint should be 1 for c");
}


void t_isprint_0x64()
{
    Assert(isprint(100) ,"isprint should be 1 for d");
}


void t_isprint_0x65()
{
    Assert(isprint(101) ,"isprint should be 1 for e");
}


void t_isprint_0x66()
{
    Assert(isprint(102) ,"isprint should be 1 for f");
}


void t_isprint_0x67()
{
    Assert(isprint(103) ,"isprint should be 1 for g");
}


void t_isprint_0x68()
{
    Assert(isprint(104) ,"isprint should be 1 for h");
}


void t_isprint_0x69()
{
    Assert(isprint(105) ,"isprint should be 1 for i");
}


void t_isprint_0x6a()
{
    Assert(isprint(106) ,"isprint should be 1 for j");
}


void t_isprint_0x6b()
{
    Assert(isprint(107) ,"isprint should be 1 for k");
}


void t_isprint_0x6c()
{
    Assert(isprint(108) ,"isprint should be 1 for l");
}


void t_isprint_0x6d()
{
    Assert(isprint(109) ,"isprint should be 1 for m");
}


void t_isprint_0x6e()
{
    Assert(isprint(110) ,"isprint should be 1 for n");
}


void t_isprint_0x6f()
{
    Assert(isprint(111) ,"isprint should be 1 for o");
}


void t_isprint_0x70()
{
    Assert(isprint(112) ,"isprint should be 1 for p");
}


void t_isprint_0x71()
{
    Assert(isprint(113) ,"isprint should be 1 for q");
}


void t_isprint_0x72()
{
    Assert(isprint(114) ,"isprint should be 1 for r");
}


void t_isprint_0x73()
{
    Assert(isprint(115) ,"isprint should be 1 for s");
}


void t_isprint_0x74()
{
    Assert(isprint(116) ,"isprint should be 1 for t");
}


void t_isprint_0x75()
{
    Assert(isprint(117) ,"isprint should be 1 for u");
}


void t_isprint_0x76()
{
    Assert(isprint(118) ,"isprint should be 1 for v");
}


void t_isprint_0x77()
{
    Assert(isprint(119) ,"isprint should be 1 for w");
}


void t_isprint_0x78()
{
    Assert(isprint(120) ,"isprint should be 1 for x");
}


void t_isprint_0x79()
{
    Assert(isprint(121) ,"isprint should be 1 for y");
}


void t_isprint_0x7a()
{
    Assert(isprint(122) ,"isprint should be 1 for z");
}


void t_isprint_0x7b()
{
    Assert(isprint(123) ,"isprint should be 1 for {");
}


void t_isprint_0x7c()
{
    Assert(isprint(124) ,"isprint should be 1 for |");
}


void t_isprint_0x7d()
{
    Assert(isprint(125) ,"isprint should be 1 for }");
}


void t_isprint_0x7e()
{
    Assert(isprint(126) ,"isprint should be 1 for ~");
}


void t_isprint_0x7f()
{
    Assert(isprint(127)  == 0 ,"isprint should be 0 for 0x7f");
}


int main(void)
{
    suite_setup("test_isprint_shard_03");
    suite_add_test(t_isprint_0x60);
    suite_add_test(t_isprint_0x61);
    suite_add_test(t_isprint_0x62);
    suite_add_test(t_isprint_0x63);
    suite_add_test(t_isprint_0x64);
    suite_add_test(t_isprint_0x65);
    suite_add_test(t_isprint_0x66);
    suite_add_test(t_isprint_0x67);
    suite_add_test(t_isprint_0x68);
    suite_add_test(t_isprint_0x69);
    suite_add_test(t_isprint_0x6a);
    suite_add_test(t_isprint_0x6b);
    suite_add_test(t_isprint_0x6c);
    suite_add_test(t_isprint_0x6d);
    suite_add_test(t_isprint_0x6e);
    suite_add_test(t_isprint_0x6f);
    suite_add_test(t_isprint_0x70);
    suite_add_test(t_isprint_0x71);
    suite_add_test(t_isprint_0x72);
    suite_add_test(t_isprint_0x73);
    suite_add_test(t_isprint_0x74);
    suite_add_test(t_isprint_0x75);
    suite_add_test(t_isprint_0x76);
    suite_add_test(t_isprint_0x77);
    suite_add_test(t_isprint_0x78);
    suite_add_test(t_isprint_0x79);
    suite_add_test(t_isprint_0x7a);
    suite_add_test(t_isprint_0x7b);
    suite_add_test(t_isprint_0x7c);
    suite_add_test(t_isprint_0x7d);
    suite_add_test(t_isprint_0x7e);
    suite_add_test(t_isprint_0x7f);
    return suite_run();
}
