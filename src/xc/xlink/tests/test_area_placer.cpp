// test_area_placer.cpp
//
// area_placer unit tests
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
// 2021-07-28   tstih
#include <xlink/area_placer.hpp>

TEST(area_placer_next_free_no_holes) {
    std::vector<xlink::address_range> holes;
    uint16_t addr = xlink::area_placer::next_free_address(0, 100, holes);
    ASSERT_EQ(addr, 0);
}

TEST(area_placer_next_free_skip_hole) {
    std::vector<xlink::address_range> holes = {{0x0010, 0x001F}};
    // Trying to place 32 bytes starting at 0: [0, 31] overlaps [16, 31].
    uint16_t addr = xlink::area_placer::next_free_address(0, 32, holes);
    ASSERT_EQ(addr, 0x0020);
}

TEST(area_placer_next_free_fits_before_hole) {
    std::vector<xlink::address_range> holes = {{0x0020, 0x002F}};
    // 16 bytes at 0: [0, 15] doesn't overlap [32, 47].
    uint16_t addr = xlink::area_placer::next_free_address(0, 16, holes);
    ASSERT_EQ(addr, 0);
}

TEST(area_placer_next_free_multiple_holes) {
    std::vector<xlink::address_range> holes = {
        {0x0000, 0x000F},
        {0x0020, 0x002F}
    };
    // 32 bytes: skip first hole to 0x10, but [0x10, 0x2F] overlaps
    // second hole, so skip to 0x30.
    uint16_t addr = xlink::area_placer::next_free_address(0, 32, holes);
    ASSERT_EQ(addr, 0x0030);
}

TEST(area_placer_con_placement) {
    xlink::link_context ctx;
    auto mod = std::make_shared<xlink::module>("test", "test.rel");
    mod->areas().emplace_back("_CODE", 0x10, xlink::area_flags::none, 0);
    mod->areas().emplace_back("_DATA", 0x08, xlink::area_flags::none, 1);
    ctx.modules.push_back(mod);

    xlink::area_placer::place(ctx);

    ASSERT(mod->areas()[0].placed_addr().has_value());
    ASSERT_EQ(mod->areas()[0].placed_addr().value(), 0);
    ASSERT(mod->areas()[1].placed_addr().has_value());
    ASSERT_EQ(mod->areas()[1].placed_addr().value(), 0x10);
    ASSERT_EQ(ctx.code_size, 0x18);
}

TEST(area_placer_ovr_placement) {
    xlink::link_context ctx;
    auto mod1 = std::make_shared<xlink::module>("m1", "m1.rel");
    mod1->areas().emplace_back("_OVR", 0x10, xlink::area_flags::ovr, 0);
    auto mod2 = std::make_shared<xlink::module>("m2", "m2.rel");
    mod2->areas().emplace_back("_OVR", 0x08, xlink::area_flags::ovr, 0);
    ctx.modules.push_back(mod1);
    ctx.modules.push_back(mod2);

    xlink::area_placer::place(ctx);

    // Both should be at same address, space = max(0x10, 0x08) = 0x10.
    ASSERT_EQ(mod1->areas()[0].placed_addr().value(),
              mod2->areas()[0].placed_addr().value());
    ASSERT_EQ(ctx.code_size, 0x10);
}

TEST(area_placer_con_with_hole) {
    xlink::link_context ctx;
    ctx.holes.push_back({0x0004, 0x000F});
    auto mod = std::make_shared<xlink::module>("test", "test.rel");
    mod->areas().emplace_back("_CODE", 0x10, xlink::area_flags::none, 0);
    ctx.modules.push_back(mod);

    xlink::area_placer::place(ctx);

    // 16 bytes starting at 0: [0, 15] overlaps [4, 15], so skip to 0x10.
    ASSERT_EQ(mod->areas()[0].placed_addr().value(), 0x10);
}
