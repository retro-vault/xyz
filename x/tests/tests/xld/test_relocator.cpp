// test_relocator.cpp
//
// relocator unit tests
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
// 2021-07-28   tstih
#include <xld/relocator.h>
#include <xld/errors.h>

TEST(relocator_copy_text_data) {
    xld::link_context ctx;
    auto mod = std::make_shared<xld::module>("test", "test.rel");
    mod->areas().emplace_back("_CODE", 4, xld::area_flags::none, 0);
    mod->areas()[0].set_placed_addr(0);

    xld::text_record tr;
    tr.area_index = 0;
    tr.offset = 0;
    tr.data = {0xC9, 0x00, 0x3E, 0x42};
    mod->texts().push_back(tr);

    ctx.modules.push_back(mod);
    ctx.code_size = 4;

    xld::relocator::relocate(ctx);

    ASSERT_EQ(static_cast<int>(ctx.code_buffer.size()), 4);
    ASSERT_EQ(ctx.code_buffer[0], 0xC9);
    ASSERT_EQ(ctx.code_buffer[1], 0x00);
    ASSERT_EQ(ctx.code_buffer[2], 0x3E);
    ASSERT_EQ(ctx.code_buffer[3], 0x42);
}

TEST(relocator_rejects_text_past_declared_area_size) {
    xld::link_context ctx;
    auto mod = std::make_shared<xld::module>("oversized", "oversized.rel");
    mod->areas().emplace_back("_CODE", 4, xld::area_flags::none, 0);
    mod->areas()[0].set_placed_addr(0);

    xld::text_record tr;
    tr.area_index = 0;
    tr.offset = 0;
    tr.data = {0x00, 0x00, 0x00, 0x00, 0x00};
    mod->texts().push_back(tr);

    ctx.modules.push_back(mod);
    ctx.code_size = 4;

    ASSERT_THROWS(xld::relocator::relocate(ctx), xld::reloc_error);
}

TEST(relocator_area_relocation) {
    xld::link_context ctx;
    auto mod = std::make_shared<xld::module>("test", "test.rel");
    mod->areas().emplace_back("_CODE", 4, xld::area_flags::none, 0);
    mod->areas()[0].set_placed_addr(0x100);

    xld::text_record tr;
    tr.area_index = 0;
    tr.offset = 0;
    // CALL 0x0000 -> should be relocated to CALL 0x0100
    tr.data = {0xCD, 0x00, 0x00, 0xC9};
    // Relocate word at offset 1, referencing area 0.
    xld::reloc_entry re;
    re.mode = xld::reloc_mode::word;  // 16-bit, area ref
    re.offset_in_t = 1;
    re.ref_index = 0;
    tr.relocs.push_back(re);
    mod->texts().push_back(tr);

    ctx.modules.push_back(mod);
    ctx.code_size = 0x104;

    xld::relocator::relocate(ctx);

    // The word at offset 0x101 should be 0x0100 (area base).
    uint16_t val = ctx.code_buffer[0x101]
                 | (ctx.code_buffer[0x102] << 8);
    ASSERT_EQ(val, 0x0100);
}

TEST(relocator_builds_reloc_table) {
    xld::link_context ctx;
    auto mod = std::make_shared<xld::module>("test", "test.rel");
    mod->areas().emplace_back("_CODE", 4, xld::area_flags::none, 0);
    mod->areas()[0].set_placed_addr(0);

    xld::text_record tr;
    tr.area_index = 0;
    tr.offset = 0;
    tr.data = {0xCD, 0x00, 0x00, 0xC9};
    xld::reloc_entry re;
    re.mode = xld::reloc_mode::word;
    re.offset_in_t = 1;
    re.ref_index = 0;
    tr.relocs.push_back(re);
    mod->texts().push_back(tr);

    ctx.modules.push_back(mod);
    ctx.code_size = 4;

    xld::relocator::relocate(ctx);

    // Should have one reloc table entry.
    ASSERT_EQ(static_cast<int>(ctx.reloc_table.size()), 1);
    ASSERT_EQ(ctx.reloc_table[0].offset, 1);
    ASSERT_EQ(ctx.reloc_table[0].size, 2);
    ASSERT_EQ(ctx.reloc_table[0].pad, 0);
}

TEST(relocator_builds_msb_byte_reloc_table_entry) {
    xld::link_context ctx;
    auto mod = std::make_shared<xld::module>("test", "test.rel");
    mod->areas().emplace_back("_CODE", 1, xld::area_flags::none, 0);
    mod->areas()[0].set_placed_addr(0x1234);

    xld::text_record tr;
    tr.area_index = 0;
    tr.offset = 0;
    tr.data = {0x00};

    xld::reloc_entry re;
    re.mode = xld::reloc_mode::msb;
    re.offset_in_t = 0;
    re.ref_index = 0;
    tr.relocs.push_back(re);
    mod->texts().push_back(tr);

    ctx.modules.push_back(mod);
    ctx.code_size = 1;

    xld::relocator::relocate(ctx);

    ASSERT_EQ(static_cast<int>(ctx.reloc_table.size()), 1);
    ASSERT_EQ(ctx.reloc_table[0].offset, 0x1234);
    ASSERT_EQ(ctx.reloc_table[0].size, 1);
    ASSERT_EQ(ctx.reloc_table[0].pad, 0x01);
    ASSERT_EQ(ctx.code_buffer[0x1234], 0x12);
}

TEST(relocator_word_relocation_writes_last_buffer_byte) {
    xld::link_context ctx;
    auto mod = std::make_shared<xld::module>("test", "test.rel");
    mod->areas().emplace_back("_CODE", 2, xld::area_flags::none, 0);
    mod->areas()[0].set_placed_addr(0x1234);

    xld::text_record tr;
    tr.area_index = 0;
    tr.offset = 0;
    tr.data = {0x00, 0x00};

    xld::reloc_entry re;
    re.mode = xld::reloc_mode::word;
    re.offset_in_t = 0;
    re.ref_index = 0;
    tr.relocs.push_back(re);
    mod->texts().push_back(tr);

    ctx.modules.push_back(mod);
    ctx.code_size = 2;

    xld::relocator::relocate(ctx);

    ASSERT_EQ(static_cast<int>(ctx.code_buffer.size()), 0x1236);
    ASSERT_EQ(ctx.code_buffer[0x1234], 0x34);
    ASSERT_EQ(ctx.code_buffer[0x1235], 0x12);
}

TEST(relocator_word_relocation_rejects_final_byte_start) {
    xld::link_context ctx;
    auto mod = std::make_shared<xld::module>("test", "test.rel");
    mod->areas().emplace_back("_CODE", 2, xld::area_flags::none, 0);
    mod->areas()[0].set_placed_addr(0x1234);

    xld::text_record tr;
    tr.area_index = 0;
    tr.offset = 0;
    tr.data = {0xAA, 0x00};

    xld::reloc_entry re;
    re.mode = xld::reloc_mode::word;
    re.offset_in_t = 1;
    re.ref_index = 0;
    tr.relocs.push_back(re);
    mod->texts().push_back(tr);

    ctx.modules.push_back(mod);
    ctx.code_size = 2;

    ASSERT_THROWS(xld::relocator::relocate(ctx), xld::reloc_error);
}

TEST(relocator_pc_relative_byte_rejects_out_of_range_target) {
    xld::link_context ctx;
    auto mod = std::make_shared<xld::module>("test", "test.rel");
    mod->areas().emplace_back("_SRC", 2, xld::area_flags::none, 0);
    mod->areas().emplace_back("_DST", 1, xld::area_flags::none, 1);
    mod->areas()[0].set_placed_addr(0x0000);
    mod->areas()[1].set_placed_addr(0x0200);

    xld::text_record tr;
    tr.area_index = 0;
    tr.offset = 0;
    tr.data = {0x18, 0x00}; // jr area1

    xld::reloc_entry re;
    re.mode = xld::reloc_mode::pc_rel;
    re.offset_in_t = 1;
    re.ref_index = 1;
    tr.relocs.push_back(re);
    mod->texts().push_back(tr);

    ctx.modules.push_back(mod);
    ctx.code_size = 0x0201;

    ASSERT_THROWS(xld::relocator::relocate(ctx), xld::reloc_error);
}

TEST(relocator_symbol_relocation_to_absolute_def_skips_area_bias) {
    xld::link_context ctx;

    auto caller = std::make_shared<xld::module>("caller", "caller.rel");
    caller->areas().emplace_back("_CODE", 4, xld::area_flags::none, 0);
    caller->areas()[0].set_placed_addr(0xC000);
    caller->symbols().emplace_back("kfunc", xld::symbol_type::ref, 0, 0);

    xld::text_record tr;
    tr.area_index = 0;
    tr.offset = 0;
    tr.data = {0xCD, 0x00, 0x00, 0xC9};

    xld::reloc_entry re;
    re.mode = xld::reloc_mode::word | xld::reloc_mode::sym;
    re.offset_in_t = 1;
    re.ref_index = 0;
    tr.relocs.push_back(re);
    caller->texts().push_back(tr);

    auto provider = std::make_shared<xld::module>("provider", "provider.rel");
    provider->areas().emplace_back("_CODE", 0, xld::area_flags::none, 0);
    provider->areas()[0].set_placed_addr(0x4000);
    provider->symbols().emplace_back("kfunc", xld::symbol_type::def,
                                     0x1234, 0, -1, true);

    ctx.modules.push_back(caller);
    ctx.modules.push_back(provider);
    ctx.global_symbols["kfunc"] = {provider.get(), 0};
    ctx.code_size = 0xC004;

    xld::relocator::relocate(ctx);

    const uint16_t value = ctx.code_buffer[0xC001]
                         | (ctx.code_buffer[0xC002] << 8);
    ASSERT_EQ(value, 0x1234);
}

// ---------------------------------------------------------------------------
// Relocation addends
//
// The addend rides on the relocation, not in the image: ASxxxx keeps it in
// the data bytes and GNU keeps it in the record, and the object readers
// normalize both. The two dialects also disagree on where a PC-relative
// relocation is measured from.
// ---------------------------------------------------------------------------

namespace reloc_addend_test {

    // One two-byte short branch at 0x0000 targeting a symbol at `target`.
    static uint8_t short_branch_displacement(int32_t addend,
                                             bool pc_rel_at_field,
                                             uint16_t target)
    {
        xld::link_context ctx;
        auto mod = std::make_shared<xld::module>("b", "b.rel");
        mod->areas().emplace_back("_CODE", 2, xld::area_flags::none, 0);
        mod->areas()[0].set_placed_addr(0x0000);
        mod->areas().emplace_back("_FAR", 1, xld::area_flags::none, 1);
        mod->areas()[1].set_placed_addr(target);
        mod->symbols().emplace_back("_t", xld::symbol_type::def, 0, 0, 1);

        xld::text_record tr;
        tr.area_index = 0;
        tr.offset = 0;
        tr.data = {0x18, 0x00};

        xld::reloc_entry re;
        re.mode = xld::reloc_mode::pc_rel | xld::reloc_mode::sym;
        re.offset_in_t = 1;
        re.ref_index = 0;
        re.addend = addend;
        re.pc_rel_at_field = pc_rel_at_field;
        tr.relocs.push_back(re);
        mod->texts().push_back(tr);

        ctx.modules.push_back(mod);
        ctx.code_size = static_cast<uint32_t>(target) + 1u;
        xld::relocator::relocate(ctx);
        return ctx.code_buffer[1];
    }

} // namespace reloc_addend_test

TEST(relocator_asxxxx_short_branch_measures_after_the_field) {
    // jr _t with _t at 0x0020: 0x0020 - 0x0002.
    ASSERT_EQ(reloc_addend_test::short_branch_displacement(0, false, 0x0020),
              0x1E);
}

TEST(relocator_gnu_short_branch_measures_at_the_field) {
    // GNU biases the addend by -1 because it measures from the displacement
    // byte, so the same branch must land on the same target.
    ASSERT_EQ(reloc_addend_test::short_branch_displacement(-1, true, 0x0020),
              0x1E);
}

TEST(relocator_short_branch_honours_a_negative_addend) {
    // jr _t-2 with _t at 0x0020 must reach 0x001E, not run off unsigned.
    ASSERT_EQ(reloc_addend_test::short_branch_displacement(-2, false, 0x0020),
              0x1C);
}

TEST(relocator_short_branch_honours_a_positive_addend) {
    ASSERT_EQ(reloc_addend_test::short_branch_displacement(3, false, 0x0020),
              0x21);
}

TEST(relocator_short_branch_backwards_with_negative_addend) {
    // Target below the branch: 0x0000 is _t, addend -0x10 is unreachable,
    // but -0x02 from a target at 0x0060 is fine in the other direction.
    ASSERT_EQ(reloc_addend_test::short_branch_displacement(-2, false, 0x0060),
              0x5C);
}

TEST(relocator_word_relocation_uses_the_relocation_addend) {
    xld::link_context ctx;
    auto mod = std::make_shared<xld::module>("w", "w.rel");
    mod->areas().emplace_back("_CODE", 3, xld::area_flags::none, 0);
    mod->areas()[0].set_placed_addr(0x0100);
    mod->symbols().emplace_back("_t", xld::symbol_type::def, 0, 0, 0);

    xld::text_record tr;
    tr.area_index = 0;
    tr.offset = 0;
    tr.data = {0xC3, 0x00, 0x00};   // GNU leaves the field empty

    xld::reloc_entry re;
    re.mode = xld::reloc_mode::word | xld::reloc_mode::sym;
    re.offset_in_t = 1;
    re.ref_index = 0;
    re.addend = 0x0004;
    tr.relocs.push_back(re);
    mod->texts().push_back(tr);

    ctx.modules.push_back(mod);
    ctx.code_size = 0x0103;
    xld::relocator::relocate(ctx);

    ASSERT_EQ(ctx.code_buffer[0x0101], 0x04); // 0x0100 + 4
    ASSERT_EQ(ctx.code_buffer[0x0102], 0x01);
}
