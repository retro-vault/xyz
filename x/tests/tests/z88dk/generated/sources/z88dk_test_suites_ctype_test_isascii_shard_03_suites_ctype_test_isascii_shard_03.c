/* auto-generated z88dk ctype shard */
#include "../../upstream/test/suites/ctype/ctype_test.h"

void t_isascii_0x60()
{
    Assert(isascii(96) ,"isascii should be 1 for `");
}


void t_isascii_0x61()
{
    Assert(isascii(97) ,"isascii should be 1 for a");
}


void t_isascii_0x62()
{
    Assert(isascii(98) ,"isascii should be 1 for b");
}


void t_isascii_0x63()
{
    Assert(isascii(99) ,"isascii should be 1 for c");
}


void t_isascii_0x64()
{
    Assert(isascii(100) ,"isascii should be 1 for d");
}


void t_isascii_0x65()
{
    Assert(isascii(101) ,"isascii should be 1 for e");
}


void t_isascii_0x66()
{
    Assert(isascii(102) ,"isascii should be 1 for f");
}


void t_isascii_0x67()
{
    Assert(isascii(103) ,"isascii should be 1 for g");
}


void t_isascii_0x68()
{
    Assert(isascii(104) ,"isascii should be 1 for h");
}


void t_isascii_0x69()
{
    Assert(isascii(105) ,"isascii should be 1 for i");
}


void t_isascii_0x6a()
{
    Assert(isascii(106) ,"isascii should be 1 for j");
}


void t_isascii_0x6b()
{
    Assert(isascii(107) ,"isascii should be 1 for k");
}


void t_isascii_0x6c()
{
    Assert(isascii(108) ,"isascii should be 1 for l");
}


void t_isascii_0x6d()
{
    Assert(isascii(109) ,"isascii should be 1 for m");
}


void t_isascii_0x6e()
{
    Assert(isascii(110) ,"isascii should be 1 for n");
}


void t_isascii_0x6f()
{
    Assert(isascii(111) ,"isascii should be 1 for o");
}


void t_isascii_0x70()
{
    Assert(isascii(112) ,"isascii should be 1 for p");
}


void t_isascii_0x71()
{
    Assert(isascii(113) ,"isascii should be 1 for q");
}


void t_isascii_0x72()
{
    Assert(isascii(114) ,"isascii should be 1 for r");
}


void t_isascii_0x73()
{
    Assert(isascii(115) ,"isascii should be 1 for s");
}


void t_isascii_0x74()
{
    Assert(isascii(116) ,"isascii should be 1 for t");
}


void t_isascii_0x75()
{
    Assert(isascii(117) ,"isascii should be 1 for u");
}


void t_isascii_0x76()
{
    Assert(isascii(118) ,"isascii should be 1 for v");
}


void t_isascii_0x77()
{
    Assert(isascii(119) ,"isascii should be 1 for w");
}


void t_isascii_0x78()
{
    Assert(isascii(120) ,"isascii should be 1 for x");
}


void t_isascii_0x79()
{
    Assert(isascii(121) ,"isascii should be 1 for y");
}


void t_isascii_0x7a()
{
    Assert(isascii(122) ,"isascii should be 1 for z");
}


void t_isascii_0x7b()
{
    Assert(isascii(123) ,"isascii should be 1 for {");
}


void t_isascii_0x7c()
{
    Assert(isascii(124) ,"isascii should be 1 for |");
}


void t_isascii_0x7d()
{
    Assert(isascii(125) ,"isascii should be 1 for }");
}


void t_isascii_0x7e()
{
    Assert(isascii(126) ,"isascii should be 1 for ~");
}


void t_isascii_0x7f()
{
    Assert(isascii(127) ,"isascii should be 1 for 0x7f");
}


int main(void)
{
    suite_setup("test_isascii_shard_03");
    suite_add_test(t_isascii_0x60);
    suite_add_test(t_isascii_0x61);
    suite_add_test(t_isascii_0x62);
    suite_add_test(t_isascii_0x63);
    suite_add_test(t_isascii_0x64);
    suite_add_test(t_isascii_0x65);
    suite_add_test(t_isascii_0x66);
    suite_add_test(t_isascii_0x67);
    suite_add_test(t_isascii_0x68);
    suite_add_test(t_isascii_0x69);
    suite_add_test(t_isascii_0x6a);
    suite_add_test(t_isascii_0x6b);
    suite_add_test(t_isascii_0x6c);
    suite_add_test(t_isascii_0x6d);
    suite_add_test(t_isascii_0x6e);
    suite_add_test(t_isascii_0x6f);
    suite_add_test(t_isascii_0x70);
    suite_add_test(t_isascii_0x71);
    suite_add_test(t_isascii_0x72);
    suite_add_test(t_isascii_0x73);
    suite_add_test(t_isascii_0x74);
    suite_add_test(t_isascii_0x75);
    suite_add_test(t_isascii_0x76);
    suite_add_test(t_isascii_0x77);
    suite_add_test(t_isascii_0x78);
    suite_add_test(t_isascii_0x79);
    suite_add_test(t_isascii_0x7a);
    suite_add_test(t_isascii_0x7b);
    suite_add_test(t_isascii_0x7c);
    suite_add_test(t_isascii_0x7d);
    suite_add_test(t_isascii_0x7e);
    suite_add_test(t_isascii_0x7f);
    return suite_run();
}
