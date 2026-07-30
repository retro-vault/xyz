// test_sys.cpp — direct tests for system-facing runtime helpers.

TEST(tls_base_returns_linked_template)
{
    REQUIRE(g_rt->call16(rt_sym::tls_base, 0, 0));
    const auto state = g_rt->snap();
    REQUIRE_EQ(state.hl, rt_sym::tls_template);
    REQUIRE_EQ(g_rt->mem.read(state.hl), 0x5a);
}
