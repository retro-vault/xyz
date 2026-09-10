// test_reserved_holes.cpp
//
// reserved address range ("hole") tests
//
// A reserved range n..m must:
//   - never receive placed code
//   - stay zero filled in flat output
//   - be preceded by a synthesized JR/JP that skips to m+1
//   - keep the guard bytes free during placement
//   - force every short branch that now spans the hole to a long form
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <xld/area_placer.h>
#include <xld/binary_emitter.h>
#include <xld/branch_relaxer.h>
#include <xld/errors.h>
#include <xld/holes.h>
#include <xld/linker.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------

namespace holes_test {

    static std::shared_ptr<xld::module> make_module(const std::string& name) {
        return std::make_shared<xld::module>(name, name + ".rel");
    }

    // Append an area plus a matching T record of `bytes`.
    static void add_area(const std::shared_ptr<xld::module>& mod,
                         const std::string& name,
                         const std::vector<uint8_t>& bytes)
    {
        const int idx = static_cast<int>(mod->areas().size());
        mod->areas().emplace_back(name,
                                  static_cast<uint16_t>(bytes.size()),
                                  xld::area_flags::none,
                                  idx);
        xld::text_record tr;
        tr.area_index = idx;
        tr.offset = 0;
        tr.data = bytes;
        mod->texts().push_back(tr);
    }

    static void add_symbol(const std::shared_ptr<xld::module>& mod,
                           const std::string& name,
                           uint16_t value,
                           int area_index)
    {
        const int idx = static_cast<int>(mod->symbols().size());
        mod->symbols().emplace_back(name, xld::symbol_type::def,
                                    value, idx, area_index);
    }

    // crt0 pulls in s__NAME / l__NAME to clear or copy a whole group.
    static void add_span_ref(const std::shared_ptr<xld::module>& mod,
                             const std::string& name)
    {
        const int idx = static_cast<int>(mod->symbols().size());
        mod->symbols().emplace_back(name, xld::symbol_type::ref, 0, idx, -1);
    }

    // The object readers put the addend on the relocation, so fixtures that
    // stand in for a parsed module must do the same.
    static void add_reloc(const std::shared_ptr<xld::module>& mod,
                          int text_index,
                          xld::reloc_mode mode,
                          uint16_t offset_in_t,
                          int ref_index,
                          int32_t addend = 0)
    {
        xld::reloc_entry re;
        re.mode = mode;
        re.offset_in_t = offset_in_t;
        re.ref_index = ref_index;
        re.addend = addend;
        mod->texts()[text_index].relocs.push_back(re);
    }

    static bool has_range(const std::vector<xld::address_range>& ranges,
                          uint16_t start, uint16_t end)
    {
        return std::any_of(ranges.begin(), ranges.end(),
                           [&](const xld::address_range& r) {
                               return r.start == start && r.end == end;
                           });
    }

    static bool in_any_hole(const std::vector<xld::address_range>& holes,
                            uint16_t addr)
    {
        return std::any_of(holes.begin(), holes.end(),
                           [&](const xld::address_range& r) {
                               return addr >= r.start && addr <= r.end;
                           });
    }

    static std::vector<uint8_t> emit_bin(const xld::link_context& ctx,
                                         const std::string& tag)
    {
        const auto path = std::filesystem::temp_directory_path()
                        / ("xld-holes-" + tag + ".bin");
        xld::binary_emitter::emit(path, ctx);
        std::ifstream in(path, std::ios::binary);
        ASSERT(in.is_open());
        std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), {});
        in.close();
        std::filesystem::remove(path);
        return bytes;
    }

    static std::string emit_text(const xld::link_context& ctx,
                                 const std::string& tag,
                                 const std::string& ext)
    {
        const auto path = std::filesystem::temp_directory_path()
                        / ("xld-holes-" + tag + ext);
        xld::binary_emitter::emit(path, ctx);
        std::ifstream in(path);
        ASSERT(in.is_open());
        std::string text((std::istreambuf_iterator<char>(in)), {});
        in.close();
        std::filesystem::remove(path);
        return text;
    }

} // namespace holes_test

// ---------------------------------------------------------------------------
// 1. guard reservation during placement
// ---------------------------------------------------------------------------

TEST(holes_xl_output_reserves_no_guard_bytes) {
    xld::link_context ctx;
    ctx.format = xld::output_format::xl;
    ctx.holes.push_back({0x0100, 0x010F});

    const auto eff = xld::area_placer::effective_holes_for_placement(ctx);
    ASSERT_EQ(static_cast<int>(eff.size()), 1);
    ASSERT(holes_test::has_range(eff, 0x0100, 0x010F));
}

TEST(holes_bin_reserves_two_guard_bytes_for_short_hole) {
    xld::link_context ctx;
    ctx.format = xld::output_format::bin;
    ctx.output_range = xld::address_range{0x0000, 0x01FF};
    ctx.holes.push_back({0x0100, 0x010F});

    const auto eff = xld::area_placer::effective_holes_for_placement(ctx);
    ASSERT(holes_test::has_range(eff, 0x0100, 0x010F));
    ASSERT(holes_test::has_range(eff, 0x00FE, 0x00FF));
}

TEST(holes_bin_reserves_three_guard_bytes_for_long_hole) {
    xld::link_context ctx;
    ctx.format = xld::output_format::bin;
    ctx.output_range = xld::address_range{0x0000, 0x0FFF};
    ctx.holes.push_back({0x0100, 0x0200}); // 0x101 bytes: too big for JR

    const auto eff = xld::area_placer::effective_holes_for_placement(ctx);
    ASSERT(holes_test::has_range(eff, 0x0100, 0x0200));
    ASSERT(holes_test::has_range(eff, 0x00FD, 0x00FF));
}

TEST(holes_bin_reserves_no_guard_when_hole_starts_at_window_start) {
    xld::link_context ctx;
    ctx.format = xld::output_format::bin;
    ctx.output_range = xld::address_range{0x8000, 0x8FFF};
    ctx.holes.push_back({0x8000, 0x800F});

    const auto eff = xld::area_placer::effective_holes_for_placement(ctx);
    ASSERT_EQ(static_cast<int>(eff.size()), 1);
}

TEST(holes_ihx_reserves_guard_bytes_like_bin) {
    xld::link_context ctx;
    ctx.format = xld::output_format::ihx;
    ctx.output_range = xld::address_range{0x0000, 0x01FF};
    ctx.holes.push_back({0x0100, 0x010F});

    const auto eff = xld::area_placer::effective_holes_for_placement(ctx);
    ASSERT(holes_test::has_range(eff, 0x00FE, 0x00FF));
}

TEST(holes_merges_overlapping_and_abutting_ranges) {
    const std::vector<xld::address_range> declared = {
        {0x0110, 0x011F},   // out of order
        {0x0100, 0x010F},   // abuts the one above
        {0x0200, 0x0210},
        {0x0208, 0x021F},   // overlaps the one above
        {0x0300, 0x0300}
    };

    const auto merged = xld::merge_reserved_ranges(declared);
    ASSERT_EQ(static_cast<int>(merged.size()), 3);
    ASSERT(holes_test::has_range(merged, 0x0100, 0x011F));
    ASSERT(holes_test::has_range(merged, 0x0200, 0x021F));
    ASSERT(holes_test::has_range(merged, 0x0300, 0x0300));
}

TEST(holes_abutting_ranges_get_a_single_guard) {
    const std::vector<xld::address_range> declared = {
        {0x0100, 0x010F}, {0x0110, 0x011F}
    };

    const auto guards =
        xld::reserved_range_guards(declared, 0x0000, 0x01FF);
    ASSERT_EQ(static_cast<int>(guards.size()), 1);
    ASSERT_EQ(guards[0].address, 0x00FE);
    ASSERT_EQ(static_cast<int>(guards[0].bytes.size()), 2);
    ASSERT_EQ(guards[0].bytes[0], 0x18);
    ASSERT_EQ(guards[0].bytes[1], 0x20);   // clears both ranges
}

TEST(holes_guard_is_never_planted_inside_another_hole) {
    // A one byte hole sits two bytes before a range that needs a three byte
    // JP.  The guard cannot start inside the earlier hole, so the unusable
    // gap between them becomes reserved too and one guard clears both.
    const std::vector<xld::address_range> declared = {
        {0x0100, 0x0100},
        {0x0103, 0x0200}   // too big for a JR
    };

    const auto guards =
        xld::reserved_range_guards(declared, 0x0000, 0x02FF);
    for (const auto& guard : guards) {
        for (size_t i = 0; i < guard.bytes.size(); ++i) {
            ASSERT(!holes_test::in_any_hole(
                declared, static_cast<uint16_t>(guard.address + i)));
        }
    }

    ASSERT_EQ(static_cast<int>(guards.size()), 1);
    ASSERT_EQ(guards[0].address, 0x00FD);
    ASSERT_EQ(guards[0].bytes[0], 0xC3);
    ASSERT_EQ(guards[0].bytes[1], 0x01);   // 0x0201
    ASSERT_EQ(guards[0].bytes[2], 0x02);
}

TEST(holes_unusable_gap_between_ranges_is_reserved_for_placement) {
    xld::link_context ctx;
    ctx.format = xld::output_format::bin;
    ctx.output_range = xld::address_range{0x0000, 0x00FF};
    ctx.holes.push_back({0x0010, 0x0010});
    ctx.holes.push_back({0x0013, 0x0020});

    auto mod = holes_test::make_module("gap");
    mod->areas().emplace_back("_A", 0x0E, xld::area_flags::none, 0);
    mod->areas().emplace_back("_B", 0x01, xld::area_flags::none, 1);
    ctx.modules.push_back(mod);

    xld::area_placer::place(ctx);

    // 0x0011..0x0012 is only two bytes of daylight between the ranges: too
    // little to be reachable, so nothing may be placed there.
    ASSERT_EQ(mod->areas()[0].placed_addr().value(), 0x0000);
    ASSERT_EQ(mod->areas()[1].placed_addr().value(), 0x0021);
}

TEST(holes_no_guard_when_hole_ends_at_top_of_address_space) {
    xld::link_context ctx;
    ctx.format = xld::output_format::bin;
    ctx.output_range = xld::address_range{0x0000, 0xFFFF};
    ctx.holes.push_back({0xFFF0, 0xFFFF});

    const auto eff = xld::area_placer::effective_holes_for_placement(ctx);
    ASSERT_EQ(static_cast<int>(eff.size()), 1);
}

// ---------------------------------------------------------------------------
// 2. placement around holes
// ---------------------------------------------------------------------------

TEST(holes_placement_skips_hole_and_guard) {
    xld::link_context ctx;
    ctx.format = xld::output_format::bin;
    ctx.output_range = xld::address_range{0x0000, 0x00FF};
    ctx.holes.push_back({0x0020, 0x002F});

    auto mod = holes_test::make_module("m");
    mod->areas().emplace_back("_A", 0x1E, xld::area_flags::none, 0);
    mod->areas().emplace_back("_B", 0x10, xld::area_flags::none, 1);
    ctx.modules.push_back(mod);

    xld::area_placer::place(ctx);

    ASSERT_EQ(mod->areas()[0].placed_addr().value(), 0x0000);
    // 0x001E..0x001F is the JR guard, so _B lands after the hole.
    ASSERT_EQ(mod->areas()[1].placed_addr().value(), 0x0030);
}

TEST(holes_placement_handles_two_holes) {
    xld::link_context ctx;
    ctx.format = xld::output_format::bin;
    ctx.output_range = xld::address_range{0x0000, 0x00FF};
    ctx.holes.push_back({0x0010, 0x001F});
    ctx.holes.push_back({0x0040, 0x004F});

    auto mod = holes_test::make_module("m");
    mod->areas().emplace_back("_A", 0x0E, xld::area_flags::none, 0);
    mod->areas().emplace_back("_B", 0x1E, xld::area_flags::none, 1);
    mod->areas().emplace_back("_C", 0x08, xld::area_flags::none, 2);
    ctx.modules.push_back(mod);

    xld::area_placer::place(ctx);

    // _A ends right at the first guard, _B fills the whole span between the
    // first hole and the second guard, _C has to clear the second hole.
    ASSERT_EQ(mod->areas()[0].placed_addr().value(), 0x0000);
    ASSERT_EQ(mod->areas()[1].placed_addr().value(), 0x0020);
    ASSERT_EQ(mod->areas()[2].placed_addr().value(), 0x0050);
}

TEST(holes_placement_rejects_abs_area_inside_hole) {
    xld::link_context ctx;
    ctx.format = xld::output_format::bin;
    ctx.holes.push_back({0x0100, 0x010F});

    auto mod = holes_test::make_module("m");
    mod->areas().emplace_back("_ABS", 0x04, xld::area_flags::abs, 0, 0x0104);
    ctx.modules.push_back(mod);

    ASSERT_THROWS(xld::area_placer::place(ctx), xld::placement_error);
}

TEST(holes_placement_single_byte_hole) {
    xld::link_context ctx;
    ctx.format = xld::output_format::bin;
    ctx.output_range = xld::address_range{0x0000, 0x00FF};
    ctx.holes.push_back({0x0038, 0x0038});

    auto mod = holes_test::make_module("m");
    mod->areas().emplace_back("_A", 0x36, xld::area_flags::none, 0);
    mod->areas().emplace_back("_B", 0x04, xld::area_flags::none, 1);
    ctx.modules.push_back(mod);

    xld::area_placer::place(ctx);

    ASSERT_EQ(mod->areas()[0].placed_addr().value(), 0x0000);
    ASSERT_EQ(mod->areas()[1].placed_addr().value(), 0x0039);
}

// ---------------------------------------------------------------------------
// 3. flat image emission
// ---------------------------------------------------------------------------

TEST(holes_bin_writes_jr_guard_and_zero_fills) {
    xld::link_context ctx;
    ctx.format = xld::output_format::bin;
    ctx.output_range = xld::address_range{0x0000, 0x002F};
    ctx.holes.push_back({0x0020, 0x002F});
    ctx.code_size = 0x0020;
    ctx.code_buffer.assign(0x0020, 0xAA);
    ctx.code_occupancy.assign(0x0020, 0x01);

    const auto image = holes_test::emit_bin(ctx, "jr-guard");
    ASSERT_EQ(static_cast<int>(image.size()), 0x30);
    ASSERT_EQ(image[0x001D], 0xAA);  // real code survives
    ASSERT_EQ(image[0x001E], 0x18);  // jr
    ASSERT_EQ(image[0x001F], 0x10);  // + hole size == 0x0030
    for (int a = 0x20; a <= 0x2F; ++a)
        ASSERT_EQ(image[a], 0x00);
}

TEST(holes_bin_writes_jp_guard_for_long_hole) {
    xld::link_context ctx;
    ctx.format = xld::output_format::bin;
    ctx.output_range = xld::address_range{0x0000, 0x0200};
    ctx.holes.push_back({0x0100, 0x0200}); // 0x101 bytes
    ctx.code_size = 0x0100;
    ctx.code_buffer.assign(0x0100, 0xAA);
    ctx.code_occupancy.assign(0x0100, 0x01);

    const auto image = holes_test::emit_bin(ctx, "jp-guard");
    ASSERT_EQ(image[0x00FC], 0xAA);
    ASSERT_EQ(image[0x00FD], 0xC3);  // jp
    ASSERT_EQ(image[0x00FE], 0x01);  // 0x0201
    ASSERT_EQ(image[0x00FF], 0x02);
    for (int a = 0x100; a <= 0x200; ++a)
        ASSERT_EQ(image[a], 0x00);
}

TEST(holes_bin_jr_guard_at_maximum_reach) {
    // A 0x7F byte hole is the largest a JR can clear.
    xld::link_context ctx;
    ctx.format = xld::output_format::bin;
    ctx.output_range = xld::address_range{0x0000, 0x00FF};
    ctx.holes.push_back({0x0010, 0x008E}); // 0x7F bytes
    ctx.code_size = 0x0010;
    ctx.code_buffer.assign(0x0010, 0xAA);
    ctx.code_occupancy.assign(0x0010, 0x01);

    const auto image = holes_test::emit_bin(ctx, "jr-max");
    ASSERT_EQ(image[0x000E], 0x18);
    ASSERT_EQ(image[0x000F], 0x7F);
}

TEST(holes_bin_switches_to_jp_one_byte_past_jr_reach) {
    xld::link_context ctx;
    ctx.format = xld::output_format::bin;
    ctx.output_range = xld::address_range{0x0000, 0x00FF};
    ctx.holes.push_back({0x0010, 0x008F}); // 0x80 bytes
    ctx.code_size = 0x0010;
    ctx.code_buffer.assign(0x0010, 0xAA);
    ctx.code_occupancy.assign(0x0010, 0x01);

    const auto image = holes_test::emit_bin(ctx, "jp-min");
    ASSERT_EQ(image[0x000D], 0xC3);
    ASSERT_EQ(image[0x000E], 0x90);
    ASSERT_EQ(image[0x000F], 0x00);
}

TEST(holes_bin_single_byte_hole_guard) {
    xld::link_context ctx;
    ctx.format = xld::output_format::bin;
    ctx.output_range = xld::address_range{0x0000, 0x000F};
    ctx.holes.push_back({0x0008, 0x0008});
    ctx.code_size = 0x0010;
    ctx.code_buffer.assign(0x0010, 0xAA);
    ctx.code_occupancy.assign(0x0010, 0x01);

    const auto image = holes_test::emit_bin(ctx, "one-byte");
    ASSERT_EQ(image[0x0006], 0x18);
    ASSERT_EQ(image[0x0007], 0x01);
    ASSERT_EQ(image[0x0008], 0x00);
    ASSERT_EQ(image[0x0009], 0xAA);
}

TEST(holes_bin_emits_guard_for_every_hole) {
    xld::link_context ctx;
    ctx.format = xld::output_format::bin;
    ctx.output_range = xld::address_range{0x0000, 0x00FF};
    ctx.holes.push_back({0x0020, 0x002F});
    ctx.holes.push_back({0x0060, 0x006F});
    ctx.code_size = 0x0100;
    ctx.code_buffer.assign(0x0100, 0xAA);
    ctx.code_occupancy.assign(0x0100, 0x01);

    const auto image = holes_test::emit_bin(ctx, "two-holes");
    ASSERT_EQ(image[0x001E], 0x18);
    ASSERT_EQ(image[0x001F], 0x10);
    ASSERT_EQ(image[0x005E], 0x18);
    ASSERT_EQ(image[0x005F], 0x10);
    for (int a = 0x20; a <= 0x2F; ++a) ASSERT_EQ(image[a], 0x00);
    for (int a = 0x60; a <= 0x6F; ++a) ASSERT_EQ(image[a], 0x00);
}

TEST(holes_bin_abutting_holes_keep_reserved_bytes_zero) {
    // Two touching ranges behave as one 0x20 byte hole: exactly one guard,
    // and no guard byte may be written into reserved space.
    xld::link_context ctx;
    ctx.format = xld::output_format::bin;
    ctx.output_range = xld::address_range{0x0000, 0x003F};
    ctx.holes.push_back({0x0010, 0x001F});
    ctx.holes.push_back({0x0020, 0x002F});
    ctx.code_size = 0x0040;
    ctx.code_buffer.assign(0x0040, 0xAA);
    ctx.code_occupancy.assign(0x0040, 0x01);

    const auto image = holes_test::emit_bin(ctx, "abutting");
    for (int a = 0x10; a <= 0x2F; ++a)
        ASSERT_EQ(image[a], 0x00);
    ASSERT_EQ(image[0x000E], 0x18);
    ASSERT_EQ(image[0x000F], 0x20);   // skip both ranges at once
}

TEST(holes_bin_overlapping_holes_keep_reserved_bytes_zero) {
    xld::link_context ctx;
    ctx.format = xld::output_format::bin;
    ctx.output_range = xld::address_range{0x0000, 0x003F};
    ctx.holes.push_back({0x0010, 0x0020});
    ctx.holes.push_back({0x0018, 0x002F});
    ctx.code_size = 0x0040;
    ctx.code_buffer.assign(0x0040, 0xAA);
    ctx.code_occupancy.assign(0x0040, 0x01);

    const auto image = holes_test::emit_bin(ctx, "overlapping");
    for (int a = 0x10; a <= 0x2F; ++a)
        ASSERT_EQ(image[a], 0x00);
    ASSERT_EQ(image[0x000E], 0x18);
    ASSERT_EQ(image[0x000F], 0x20);
}

TEST(holes_bin_no_guard_when_hole_touches_top_of_memory) {
    xld::link_context ctx;
    ctx.format = xld::output_format::bin;
    ctx.output_range = xld::address_range{0xFFF0, 0xFFFF};
    ctx.holes.push_back({0xFFF8, 0xFFFF});
    ctx.code_size = 0x10000;
    ctx.code_buffer.assign(0x10000, 0xAA);
    ctx.code_occupancy.assign(0x10000, 0x01);

    const auto image = holes_test::emit_bin(ctx, "top");
    ASSERT_EQ(static_cast<int>(image.size()), 0x10);
    ASSERT_EQ(image[0x0006], 0xAA);   // no guard fits
    ASSERT_EQ(image[0x0007], 0xAA);
    for (int i = 8; i < 0x10; ++i)
        ASSERT_EQ(image[i], 0x00);
}

TEST(holes_ihx_skips_reserved_bytes_and_keeps_guard) {
    xld::link_context ctx;
    ctx.format = xld::output_format::ihx;
    ctx.output_range = xld::address_range{0x0000, 0x002F};
    ctx.holes.push_back({0x0020, 0x002F});
    ctx.code_size = 0x0020;
    ctx.code_buffer.assign(0x0020, 0xAA);
    ctx.code_occupancy.assign(0x0020, 0x01);

    const auto text = holes_test::emit_text(ctx, "ihx", ".ihx");
    // The guard must be part of the loaded data ...
    ASSERT(text.find("1810") != std::string::npos);
    // ... and the reserved range must not be written at all.
    ASSERT(text.find(":100020") == std::string::npos);
    ASSERT(text.find(":00000001FF") != std::string::npos);
}

// ---------------------------------------------------------------------------
// 4. branch relaxation across holes
// ---------------------------------------------------------------------------

TEST(holes_promote_conditional_forward_branch) {
    xld::link_context ctx;
    ctx.entry_name = "_main";
    ctx.format = xld::output_format::bin;
    ctx.output_range = xld::address_range{0x0000, 0x01FF};
    ctx.holes.push_back({0x0100, 0x017F});
    ctx.area_bases["_A"] = 0x0000;
    ctx.area_bases["_B"] = 0x0180;

    auto mod = holes_test::make_module("cnd");
    holes_test::add_area(mod, "_A", {0x20, 0x00, 0xC9}); // jr nz,_t ; ret
    holes_test::add_area(mod, "_B", {0xC9});
    holes_test::add_symbol(mod, "_main", 0, 0);
    holes_test::add_symbol(mod, "_target", 0, 1);
    holes_test::add_reloc(mod, 0,
                          xld::reloc_mode::pc_rel | xld::reloc_mode::sym,
                          1, 1);
    ctx.modules.push_back(mod);

    xld::cli_options opts;
    xld::linker::link(ctx, opts);

    ASSERT_EQ(mod->area_by_index(0).size(), 4);
    ASSERT_EQ(ctx.code_buffer[0x0000], 0xC2); // jp nz
    ASSERT_EQ(ctx.code_buffer[0x0001], 0x80);
    ASSERT_EQ(ctx.code_buffer[0x0002], 0x01);
    ASSERT_EQ(ctx.code_buffer[0x0003], 0xC9);
}

TEST(holes_promote_conditional_backward_branch) {
    xld::link_context ctx;
    ctx.entry_name = "_main";
    ctx.format = xld::output_format::bin;
    ctx.output_range = xld::address_range{0x0000, 0x01FF};
    ctx.holes.push_back({0x0100, 0x017F});
    ctx.area_bases["_A"] = 0x0000;
    ctx.area_bases["_B"] = 0x0180;

    auto mod = holes_test::make_module("cndb");
    holes_test::add_area(mod, "_A", {0xC9});
    holes_test::add_area(mod, "_B", {0x38, 0x00, 0xC9}); // jr c,_t ; ret
    holes_test::add_symbol(mod, "_target", 0, 0);
    holes_test::add_symbol(mod, "_main", 0, 1);
    holes_test::add_reloc(mod, 1,
                          xld::reloc_mode::pc_rel | xld::reloc_mode::sym,
                          1, 0);
    ctx.modules.push_back(mod);

    xld::cli_options opts;
    xld::linker::link(ctx, opts);

    ASSERT_EQ(mod->area_by_index(1).size(), 4);
    ASSERT_EQ(ctx.code_buffer[0x0180], 0xDA); // jp c
    ASSERT_EQ(ctx.code_buffer[0x0181], 0x00);
    ASSERT_EQ(ctx.code_buffer[0x0182], 0x00);
    ASSERT_EQ(ctx.code_buffer[0x0183], 0xC9);
}

TEST(holes_promote_djnz_over_hole) {
    xld::link_context ctx;
    ctx.entry_name = "_main";
    ctx.format = xld::output_format::bin;
    ctx.output_range = xld::address_range{0x0000, 0x01FF};
    ctx.holes.push_back({0x0100, 0x017F});
    ctx.area_bases["_A"] = 0x0000;
    ctx.area_bases["_B"] = 0x0180;

    auto mod = holes_test::make_module("djnz");
    holes_test::add_area(mod, "_A", {0xC9});
    holes_test::add_area(mod, "_B", {0x10, 0x00, 0xC9}); // djnz _t ; ret
    holes_test::add_symbol(mod, "_target", 0, 0);
    holes_test::add_symbol(mod, "_main", 0, 1);
    holes_test::add_reloc(mod, 1,
                          xld::reloc_mode::pc_rel | xld::reloc_mode::sym,
                          1, 0);
    ctx.modules.push_back(mod);

    xld::cli_options opts;
    xld::linker::link(ctx, opts);

    // djnz +2 / jr +3 / jp _target -- 7 bytes replacing 2.
    ASSERT_EQ(mod->area_by_index(1).size(), 8);
    ASSERT_EQ(ctx.code_buffer[0x0180], 0x10);
    ASSERT_EQ(ctx.code_buffer[0x0181], 0x02);
    ASSERT_EQ(ctx.code_buffer[0x0182], 0x18);
    ASSERT_EQ(ctx.code_buffer[0x0183], 0x03);
    ASSERT_EQ(ctx.code_buffer[0x0184], 0xC3);
    ASSERT_EQ(ctx.code_buffer[0x0185], 0x00);
    ASSERT_EQ(ctx.code_buffer[0x0186], 0x00);
    ASSERT_EQ(ctx.code_buffer[0x0187], 0xC9);
}

TEST(holes_short_branch_over_small_hole_stays_short) {
    xld::link_context ctx;
    ctx.entry_name = "_main";
    ctx.format = xld::output_format::bin;
    ctx.output_range = xld::address_range{0x0000, 0x00FF};
    ctx.holes.push_back({0x0010, 0x001F});
    ctx.area_bases["_A"] = 0x0000;
    ctx.area_bases["_B"] = 0x0020;

    auto mod = holes_test::make_module("stay");
    holes_test::add_area(mod, "_A", {0x18, 0x00, 0xC9});
    holes_test::add_area(mod, "_B", {0xC9});
    holes_test::add_symbol(mod, "_main", 0, 0);
    holes_test::add_symbol(mod, "_target", 0, 1);
    holes_test::add_reloc(mod, 0,
                          xld::reloc_mode::pc_rel | xld::reloc_mode::sym,
                          1, 1);
    ctx.modules.push_back(mod);

    xld::cli_options opts;
    xld::linker::link(ctx, opts);

    ASSERT_EQ(mod->area_by_index(0).size(), 3);
    ASSERT_EQ(ctx.code_buffer[0x0000], 0x18);
    ASSERT_EQ(ctx.code_buffer[0x0001], 0x1E); // 0x0002 + 0x1E == 0x0020
    ASSERT_EQ(ctx.code_buffer[0x0002], 0xC9);
}

TEST(holes_promoted_branch_shifts_following_symbols) {
    xld::link_context ctx;
    ctx.entry_name = "_main";
    ctx.format = xld::output_format::bin;
    ctx.output_range = xld::address_range{0x0000, 0x01FF};
    ctx.holes.push_back({0x0100, 0x017F});
    ctx.area_bases["_A"] = 0x0000;
    ctx.area_bases["_B"] = 0x0180;

    auto mod = holes_test::make_module("shift");
    // jr _target ; nop ; ret     ("_after" labels the ret)
    holes_test::add_area(mod, "_A", {0x18, 0x00, 0x00, 0xC9});
    holes_test::add_area(mod, "_B", {0xC9});
    holes_test::add_symbol(mod, "_main", 0, 0);
    holes_test::add_symbol(mod, "_after", 3, 0);
    holes_test::add_symbol(mod, "_target", 0, 1);
    holes_test::add_reloc(mod, 0,
                          xld::reloc_mode::pc_rel | xld::reloc_mode::sym,
                          1, 2);
    ctx.modules.push_back(mod);

    xld::cli_options opts;
    xld::linker::link(ctx, opts);

    // The jr grew by one byte, so _after slid from 3 to 4.
    ASSERT_EQ(mod->symbol_by_index(1).value(), 4);
    ASSERT_EQ(ctx.code_buffer[0x0004], 0xC9);
}

TEST(holes_absolute_call_across_hole_is_unaffected) {
    xld::link_context ctx;
    ctx.entry_name = "_main";
    ctx.format = xld::output_format::bin;
    ctx.output_range = xld::address_range{0x0000, 0x01FF};
    ctx.holes.push_back({0x0100, 0x017F});
    ctx.area_bases["_A"] = 0x0000;
    ctx.area_bases["_B"] = 0x0180;

    auto mod = holes_test::make_module("call");
    holes_test::add_area(mod, "_A", {0xCD, 0x00, 0x00, 0xC9}); // call _t ; ret
    holes_test::add_area(mod, "_B", {0xC9});
    holes_test::add_symbol(mod, "_main", 0, 0);
    holes_test::add_symbol(mod, "_target", 0, 1);
    holes_test::add_reloc(mod, 0,
                          xld::reloc_mode::word | xld::reloc_mode::sym,
                          1, 1);
    ctx.modules.push_back(mod);

    xld::cli_options opts;
    xld::linker::link(ctx, opts);

    ASSERT_EQ(ctx.code_buffer[0x0000], 0xCD);
    ASSERT_EQ(ctx.code_buffer[0x0001], 0x80);
    ASSERT_EQ(ctx.code_buffer[0x0002], 0x01);
}

TEST(holes_promote_cross_module_branch) {
    xld::link_context ctx;
    ctx.entry_name = "_main";
    ctx.format = xld::output_format::bin;
    ctx.output_range = xld::address_range{0x0000, 0x01FF};
    ctx.holes.push_back({0x0100, 0x017F});
    ctx.area_bases["_A"] = 0x0000;
    ctx.area_bases["_B"] = 0x0180;

    auto caller = holes_test::make_module("caller");
    holes_test::add_area(caller, "_A", {0x18, 0x00, 0xC9});
    holes_test::add_symbol(caller, "_main", 0, 0);
    caller->symbols().emplace_back("_target", xld::symbol_type::ref,
                                   0, 1, -1);
    holes_test::add_reloc(caller, 0,
                          xld::reloc_mode::pc_rel | xld::reloc_mode::sym,
                          1, 1);

    auto callee = holes_test::make_module("callee");
    holes_test::add_area(callee, "_B", {0xC9});
    holes_test::add_symbol(callee, "_target", 0, 0);

    ctx.modules.push_back(caller);
    ctx.modules.push_back(callee);

    xld::cli_options opts;
    xld::linker::link(ctx, opts);

    ASSERT_EQ(caller->area_by_index(0).size(), 4);
    ASSERT_EQ(ctx.code_buffer[0x0000], 0xC3);
    ASSERT_EQ(ctx.code_buffer[0x0001], 0x80);
    ASSERT_EQ(ctx.code_buffer[0x0002], 0x01);
}

TEST(holes_entry_point_follows_promoted_branches) {
    xld::link_context ctx;
    ctx.entry_name = "_main";
    ctx.format = xld::output_format::bin;
    ctx.output_range = xld::address_range{0x0000, 0x01FF};
    ctx.holes.push_back({0x0100, 0x017F});
    ctx.area_bases["_A"] = 0x0000;
    ctx.area_bases["_B"] = 0x0180;

    auto mod = holes_test::make_module("entry");
    // jr _target ; _main: ret
    holes_test::add_area(mod, "_A", {0x18, 0x00, 0xC9});
    holes_test::add_area(mod, "_B", {0xC9});
    holes_test::add_symbol(mod, "_main", 2, 0);
    holes_test::add_symbol(mod, "_target", 0, 1);
    holes_test::add_reloc(mod, 0,
                          xld::reloc_mode::pc_rel | xld::reloc_mode::sym,
                          1, 1);
    ctx.modules.push_back(mod);

    xld::cli_options opts;
    xld::linker::link(ctx, opts);

    ASSERT_EQ(ctx.entry_point, 0x0003);
    ASSERT_EQ(ctx.code_buffer[0x0003], 0xC9);
}

TEST(holes_multiple_holes_promote_multiple_branches) {
    xld::link_context ctx;
    ctx.entry_name = "_main";
    ctx.format = xld::output_format::bin;
    ctx.output_range = xld::address_range{0x0000, 0x03FF};
    ctx.holes.push_back({0x0100, 0x017F});
    ctx.holes.push_back({0x0200, 0x027F});
    ctx.area_bases["_A"] = 0x0000;
    ctx.area_bases["_B"] = 0x0180;
    ctx.area_bases["_C"] = 0x0280;

    auto mod = holes_test::make_module("multi");
    holes_test::add_area(mod, "_A", {0x18, 0x00, 0xC9});   // jr _b1
    holes_test::add_area(mod, "_B", {0x20, 0x00, 0xC9});   // jr nz,_c1
    holes_test::add_area(mod, "_C", {0xC9});
    holes_test::add_symbol(mod, "_main", 0, 0);
    holes_test::add_symbol(mod, "_b1", 0, 1);
    holes_test::add_symbol(mod, "_c1", 0, 2);
    holes_test::add_reloc(mod, 0,
                          xld::reloc_mode::pc_rel | xld::reloc_mode::sym,
                          1, 1);
    holes_test::add_reloc(mod, 1,
                          xld::reloc_mode::pc_rel | xld::reloc_mode::sym,
                          1, 2);
    ctx.modules.push_back(mod);

    xld::cli_options opts;
    xld::linker::link(ctx, opts);

    ASSERT_EQ(ctx.code_buffer[0x0000], 0xC3);
    ASSERT_EQ(ctx.code_buffer[0x0001], 0x80);
    ASSERT_EQ(ctx.code_buffer[0x0002], 0x01);
    ASSERT_EQ(ctx.code_buffer[0x0180], 0xC2);
    ASSERT_EQ(ctx.code_buffer[0x0181], 0x80);
    ASSERT_EQ(ctx.code_buffer[0x0182], 0x02);
}

TEST(holes_two_branches_in_one_area_both_promote) {
    xld::link_context ctx;
    ctx.entry_name = "_main";
    ctx.format = xld::output_format::bin;
    ctx.output_range = xld::address_range{0x0000, 0x01FF};
    ctx.holes.push_back({0x0100, 0x017F});
    ctx.area_bases["_A"] = 0x0000;
    ctx.area_bases["_B"] = 0x0180;

    auto mod = holes_test::make_module("pair");
    // jr _t1 ; jr nz,_t2 ; ret
    holes_test::add_area(mod, "_A", {0x18, 0x00, 0x20, 0x00, 0xC9});
    holes_test::add_area(mod, "_B", {0xC9, 0xC9});
    holes_test::add_symbol(mod, "_main", 0, 0);
    holes_test::add_symbol(mod, "_t1", 0, 1);
    holes_test::add_symbol(mod, "_t2", 1, 1);
    holes_test::add_reloc(mod, 0,
                          xld::reloc_mode::pc_rel | xld::reloc_mode::sym,
                          1, 1);
    holes_test::add_reloc(mod, 0,
                          xld::reloc_mode::pc_rel | xld::reloc_mode::sym,
                          3, 2);
    ctx.modules.push_back(mod);

    xld::cli_options opts;
    xld::linker::link(ctx, opts);

    ASSERT_EQ(mod->area_by_index(0).size(), 7);
    ASSERT_EQ(ctx.code_buffer[0x0000], 0xC3);
    ASSERT_EQ(ctx.code_buffer[0x0001], 0x80);
    ASSERT_EQ(ctx.code_buffer[0x0002], 0x01);
    ASSERT_EQ(ctx.code_buffer[0x0003], 0xC2);
    ASSERT_EQ(ctx.code_buffer[0x0004], 0x81);
    ASSERT_EQ(ctx.code_buffer[0x0005], 0x01);
    ASSERT_EQ(ctx.code_buffer[0x0006], 0xC9);
}

TEST(holes_end_to_end_guard_does_not_overwrite_linked_code) {
    // Full pipeline: whatever the linker places must survive emission.
    xld::link_context ctx;
    ctx.entry_name = "_main";
    ctx.format = xld::output_format::bin;
    ctx.output_range = xld::address_range{0x0000, 0x003F};
    ctx.holes.push_back({0x0020, 0x002F});

    auto mod = holes_test::make_module("e2e");
    holes_test::add_area(mod, "_A", std::vector<uint8_t>(0x1E, 0x76));
    holes_test::add_area(mod, "_B", std::vector<uint8_t>(0x08, 0x76));
    holes_test::add_symbol(mod, "_main", 0, 0);
    ctx.modules.push_back(mod);

    xld::cli_options opts;
    xld::linker::link(ctx, opts);

    ASSERT_EQ(mod->area_by_index(0).placed_addr().value(), 0x0000);
    ASSERT_EQ(mod->area_by_index(1).placed_addr().value(), 0x0030);

    const auto image = holes_test::emit_bin(ctx, "e2e");
    for (int a = 0x00; a <= 0x1D; ++a)
        ASSERT_EQ(image[a], 0x76);
    ASSERT_EQ(image[0x1E], 0x18);
    ASSERT_EQ(image[0x1F], 0x10);
    for (int a = 0x20; a <= 0x2F; ++a)
        ASSERT_EQ(image[a], 0x00);
    for (int a = 0x30; a <= 0x37; ++a)
        ASSERT_EQ(image[a], 0x76);
}

// ---------------------------------------------------------------------------
// 5. branch addends survive promotion
// ---------------------------------------------------------------------------

TEST(holes_promoted_branch_keeps_positive_addend) {
    xld::link_context ctx;
    ctx.entry_name = "_main";
    ctx.format = xld::output_format::bin;
    ctx.output_range = xld::address_range{0x0000, 0x01FF};
    ctx.holes.push_back({0x0100, 0x017F});
    ctx.area_bases["_A"] = 0x0000;
    ctx.area_bases["_B"] = 0x0180;

    auto mod = holes_test::make_module("addpos");
    holes_test::add_area(mod, "_A", {0x18, 0x02, 0xC9}); // jr _target+2
    holes_test::add_area(mod, "_B", {0x00, 0x00, 0xC9});
    holes_test::add_symbol(mod, "_main", 0, 0);
    holes_test::add_symbol(mod, "_target", 0, 1);
    holes_test::add_reloc(mod, 0,
                          xld::reloc_mode::pc_rel | xld::reloc_mode::sym,
                          1, 1, 2);
    ctx.modules.push_back(mod);

    xld::cli_options opts;
    xld::linker::link(ctx, opts);

    ASSERT_EQ(ctx.code_buffer[0x0000], 0xC3);
    ASSERT_EQ(ctx.code_buffer[0x0001], 0x82); // 0x0180 + 2
    ASSERT_EQ(ctx.code_buffer[0x0002], 0x01);
}

TEST(holes_promoted_branch_keeps_negative_addend) {
    xld::link_context ctx;
    ctx.entry_name = "_main";
    ctx.format = xld::output_format::bin;
    ctx.output_range = xld::address_range{0x0000, 0x01FF};
    ctx.holes.push_back({0x0100, 0x017F});
    ctx.area_bases["_A"] = 0x0000;
    ctx.area_bases["_B"] = 0x0180;

    auto mod = holes_test::make_module("addneg");
    // jr _target-1 : the displacement byte carries a two's complement addend.
    holes_test::add_area(mod, "_A", {0x18, 0xFF, 0xC9});
    holes_test::add_area(mod, "_B", {0xC9, 0xC9});
    holes_test::add_symbol(mod, "_main", 0, 0);
    holes_test::add_symbol(mod, "_target", 1, 1);  // 0x0181
    holes_test::add_reloc(mod, 0,
                          xld::reloc_mode::pc_rel | xld::reloc_mode::sym,
                          1, 1, -1);
    ctx.modules.push_back(mod);

    xld::cli_options opts;
    xld::linker::link(ctx, opts);

    // _target is 0x0181, so _target-1 is 0x0180.
    ASSERT_EQ(ctx.code_buffer[0x0000], 0xC3);
    ASSERT_EQ(ctx.code_buffer[0x0001], 0x80);
    ASSERT_EQ(ctx.code_buffer[0x0002], 0x01);
}

TEST(holes_promoted_djnz_keeps_negative_addend) {
    xld::link_context ctx;
    ctx.entry_name = "_main";
    ctx.format = xld::output_format::bin;
    ctx.output_range = xld::address_range{0x0000, 0x01FF};
    ctx.holes.push_back({0x0100, 0x017F});
    ctx.area_bases["_A"] = 0x0000;
    ctx.area_bases["_B"] = 0x0180;

    auto mod = holes_test::make_module("djneg");
    holes_test::add_area(mod, "_A", {0xC9, 0xC9});
    holes_test::add_area(mod, "_B", {0x10, 0xFF, 0xC9}); // djnz _target-1
    holes_test::add_symbol(mod, "_target", 1, 0);
    holes_test::add_symbol(mod, "_main", 0, 1);
    holes_test::add_reloc(mod, 1,
                          xld::reloc_mode::pc_rel | xld::reloc_mode::sym,
                          1, 0, -1);
    ctx.modules.push_back(mod);

    xld::cli_options opts;
    xld::linker::link(ctx, opts);

    ASSERT_EQ(ctx.code_buffer[0x0184], 0xC3);
    ASSERT_EQ(ctx.code_buffer[0x0185], 0x00); // _target(0x0001) - 1 == 0x0000
    ASSERT_EQ(ctx.code_buffer[0x0186], 0x00);
}

TEST(holes_outside_the_emit_window_still_block_placement) {
    // -x only crops the file. A reserved range reaching past the window is
    // still real memory that nothing may be placed in.
    xld::link_context ctx;
    ctx.format = xld::output_format::bin;
    ctx.output_range = xld::address_range{0x0000, 0x001F};
    ctx.holes.push_back({0x0010, 0x00FF});

    auto mod = holes_test::make_module("beyond");
    mod->areas().emplace_back("_A", 0x0D, xld::area_flags::none, 0);
    mod->areas().emplace_back("_B", 0x04, xld::area_flags::none, 1);
    ctx.modules.push_back(mod);

    xld::area_placer::place(ctx);

    ASSERT_EQ(mod->areas()[0].placed_addr().value(), 0x0000);
    ASSERT_EQ(mod->areas()[1].placed_addr().value(), 0x0100);
}

TEST(holes_clipped_by_the_window_guard_the_visible_part) {
    // Only 0x0010..0x001F is emitted, so the guard skips to 0x0020 even
    // though the declared range runs further.
    const std::vector<xld::address_range> declared = {{0x0010, 0x00FF}};

    const auto guards =
        xld::reserved_range_guards(declared, 0x0000, 0x001F);
    ASSERT_EQ(static_cast<int>(guards.size()), 1);
    ASSERT_EQ(guards[0].address, 0x000E);
    ASSERT_EQ(guards[0].bytes[0], 0x18);
    ASSERT_EQ(guards[0].bytes[1], 0x10);
}

TEST(holes_starting_before_the_window_get_no_guard) {
    const std::vector<xld::address_range> declared = {{0x7FF0, 0x8010}};

    const auto guards =
        xld::reserved_range_guards(declared, 0x8000, 0x8FFF);
    ASSERT_EQ(static_cast<int>(guards.size()), 0);

    const auto clipped =
        xld::clipped_reserved_ranges(declared, 0x8000, 0x8FFF);
    ASSERT_EQ(static_cast<int>(clipped.size()), 1);
    ASSERT(holes_test::has_range(clipped, 0x8000, 0x8010));
}

TEST(holes_ihx_never_writes_reserved_bytes) {
    // Even if something claimed the reserved span, a sparse image must not
    // carry it: the flat image zero fills it, so IHX has to skip it.
    xld::link_context ctx;
    ctx.format = xld::output_format::ihx;
    ctx.output_range = xld::address_range{0x0000, 0x003F};
    ctx.holes.push_back({0x0020, 0x002F});
    ctx.code_size = 0x0040;
    ctx.code_buffer.assign(0x0040, 0xAA);
    ctx.code_occupancy.assign(0x0040, 0x01);

    const auto text = holes_test::emit_text(ctx, "ihx-skip", ".ihx");
    ASSERT(text.find(":10002000") == std::string::npos);
    ASSERT(text.find(":10003000") != std::string::npos);
}

// ---------------------------------------------------------------------------
// 7. data areas around holes
// ---------------------------------------------------------------------------

TEST(holes_data_area_is_moved_whole_not_split) {
    // An area is the unit of placement: one that does not fit before a
    // reserved range moves past it in one piece, never split around it.
    xld::link_context ctx;
    ctx.format = xld::output_format::bin;
    ctx.output_range = xld::address_range{0x0000, 0x00FF};
    ctx.holes.push_back({0x0020, 0x002F});

    auto mod = holes_test::make_module("table");
    mod->areas().emplace_back("_CODE", 0x10, xld::area_flags::none, 0);
    mod->areas().emplace_back("_CONST", 0x20, xld::area_flags::none, 1);
    ctx.modules.push_back(mod);

    xld::area_placer::place(ctx);

    ASSERT_EQ(mod->areas()[0].placed_addr().value(), 0x0000);
    // 0x0010..0x001D is free but only 0x0E bytes, so the whole 0x20 byte
    // table clears the range instead of straddling it.
    ASSERT_EQ(mod->areas()[1].placed_addr().value(), 0x0030);
}

TEST(holes_bss_span_symbols_do_not_cover_reserved_bytes) {
    // crt0 zeroes s__BSS .. s__BSS + l__BSS.  If a reserved range splits the
    // group, that span must not reach across it.
    xld::link_context ctx;
    ctx.entry_name = "_main";
    ctx.format = xld::output_format::bin;
    ctx.output_range = xld::address_range{0x0000, 0x00FF};
    ctx.holes.push_back({0x0020, 0x002F});

    auto code = holes_test::make_module("crt0");
    holes_test::add_area(code, "_CODE", {0xC9});
    holes_test::add_symbol(code, "_main", 0, 0);
    holes_test::add_span_ref(code, "s__BSS");
    holes_test::add_span_ref(code, "l__BSS");

    auto first = holes_test::make_module("bss1");
    first->areas().emplace_back("_BSS", 0x1C, xld::area_flags::never_load, 0);

    auto second = holes_test::make_module("bss2");
    second->areas().emplace_back("_BSS", 0x08, xld::area_flags::never_load, 0);

    ctx.modules.push_back(code);
    ctx.modules.push_back(first);
    ctx.modules.push_back(second);

    xld::cli_options opts;
    xld::linker::link(ctx, opts);

    const uint16_t start = ctx.linker_symbols["s__BSS"];
    const uint16_t len = ctx.linker_symbols["l__BSS"];
    ASSERT_EQ(len, 0x24);   // 0x1C + 0x08, no reserved bytes folded in

    for (uint32_t addr = start; addr < start + len; ++addr) {
        ASSERT(!holes_test::in_any_hole(
            ctx.holes, static_cast<uint16_t>(addr)));
    }
}

TEST(holes_data_span_symbols_do_not_cover_reserved_bytes) {
    xld::link_context ctx;
    ctx.entry_name = "_main";
    ctx.format = xld::output_format::bin;
    ctx.output_range = xld::address_range{0x0000, 0x00FF};
    ctx.holes.push_back({0x0020, 0x002F});

    auto code = holes_test::make_module("crt0");
    holes_test::add_area(code, "_CODE", {0xC9});
    holes_test::add_symbol(code, "_main", 0, 0);
    holes_test::add_span_ref(code, "s__DATA");
    holes_test::add_span_ref(code, "l__DATA");

    auto first = holes_test::make_module("data1");
    holes_test::add_area(first, "_DATA", std::vector<uint8_t>(0x1C, 0x11));

    auto second = holes_test::make_module("data2");
    holes_test::add_area(second, "_DATA", std::vector<uint8_t>(0x08, 0x22));

    ctx.modules.push_back(code);
    ctx.modules.push_back(first);
    ctx.modules.push_back(second);

    xld::cli_options opts;
    xld::linker::link(ctx, opts);

    const uint16_t start = ctx.linker_symbols["s__DATA"];
    const uint16_t len = ctx.linker_symbols["l__DATA"];
    ASSERT_EQ(len, 0x24);

    for (uint32_t addr = start; addr < start + len; ++addr) {
        ASSERT(!holes_test::in_any_hole(
            ctx.holes, static_cast<uint16_t>(addr)));
    }
}

TEST(holes_code_group_may_still_span_a_hole) {
    // Nothing consumes s__CODE / l__CODE, so _CODE is free to sit on both
    // sides of a reserved range -- that is what the pre-hole jump is for.
    // Forcing it contiguous would push whole programs past every range.
    xld::link_context ctx;
    ctx.entry_name = "_main";
    ctx.format = xld::output_format::bin;
    ctx.output_range = xld::address_range{0x0000, 0x00FF};
    ctx.holes.push_back({0x0020, 0x002F});

    auto first = holes_test::make_module("c1");
    holes_test::add_area(first, "_CODE", std::vector<uint8_t>(0x1E, 0x00));
    holes_test::add_symbol(first, "_main", 0, 0);

    auto second = holes_test::make_module("c2");
    holes_test::add_area(second, "_CODE", {0xC9});

    ctx.modules.push_back(first);
    ctx.modules.push_back(second);

    xld::cli_options opts;
    xld::linker::link(ctx, opts);

    ASSERT_EQ(first->area_by_index(0).placed_addr().value(), 0x0000);
    ASSERT_EQ(second->area_by_index(0).placed_addr().value(), 0x0030);

    const auto image = holes_test::emit_bin(ctx, "code-span");
    ASSERT_EQ(image[0x001E], 0x18);   // the guard carries execution across
    ASSERT_EQ(image[0x001F], 0x10);
    ASSERT_EQ(image[0x0030], 0xC9);
}

TEST(holes_span_consumer_moves_group_whole_past_the_hole) {
    // The group does not fit before the range, so all of it moves past --
    // it is never split with a jump in the middle.
    xld::link_context ctx;
    ctx.entry_name = "_main";
    ctx.format = xld::output_format::bin;
    ctx.output_range = xld::address_range{0x0000, 0x00FF};
    ctx.holes.push_back({0x0020, 0x002F});

    auto crt0 = holes_test::make_module("crt0");
    holes_test::add_area(crt0, "_CODE", {0xC9});
    holes_test::add_symbol(crt0, "_main", 0, 0);
    holes_test::add_span_ref(crt0, "l__CONST");

    auto tbl1 = holes_test::make_module("t1");
    holes_test::add_area(tbl1, "_CONST", std::vector<uint8_t>(0x10, 0x11));
    auto tbl2 = holes_test::make_module("t2");
    holes_test::add_area(tbl2, "_CONST", std::vector<uint8_t>(0x10, 0x22));

    ctx.modules.push_back(crt0);
    ctx.modules.push_back(tbl1);
    ctx.modules.push_back(tbl2);

    xld::cli_options opts;
    xld::linker::link(ctx, opts);

    ASSERT_EQ(ctx.linker_symbols["s__CONST"], 0x0030);
    ASSERT_EQ(ctx.linker_symbols["l__CONST"], 0x20);
    ASSERT_EQ(tbl1->area_by_index(0).placed_addr().value(), 0x0030);
    ASSERT_EQ(tbl2->area_by_index(0).placed_addr().value(), 0x0040);
}
