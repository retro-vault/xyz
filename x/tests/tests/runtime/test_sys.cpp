// test_sys.cpp — direct tests for system-facing runtime helpers.

#include <array>

#include "runtime_symbols.hpp"

TEST(tls_base_returns_linked_template)
{
    REQUIRE(g_rt->call16(rt_sym::tls_base, 0, 0));
    const auto state = g_rt->snap();
    REQUIRE_EQ(state.hl, rt_sym::tls_template);
    REQUIRE_EQ(g_rt->mem.read(state.hl), 0x5a);
}

TEST(cpm3_gettimeofday_preserves_destination_and_converts_clock)
{
    // CP/M invokes BDOS through address 5.  Replace the otherwise unrelated
    // runtime bytes there with a tiny function-105 mock that writes day zero
    // and 12:34 into the caller's record, returns 56 seconds in A, and RETs.
    static constexpr std::array<uint8_t, 15> bdos_stub = {
        0xaf,             // xor a
        0x12, 0x13,       // ld (de),a; inc de
        0x12, 0x13,       // ld (de),a; inc de
        0x3e, 0x12, 0x12, // ld a,#0x12; ld (de),a
        0x13,             // inc de
        0x3e, 0x34, 0x12, // ld a,#0x34; ld (de),a
        0x3e, 0x56, 0xc9  // ld a,#0x56; ret
    };
    std::array<uint8_t, bdos_stub.size()> saved{};
    for (std::size_t i = 0; i < bdos_stub.size(); ++i) {
        saved[i] = g_rt->mem.read(static_cast<uint16_t>(5 + i));
        g_rt->mem.write(static_cast<uint16_t>(5 + i), bdos_stub[i]);
    }

    static constexpr uint16_t destination = 0x6000;
    for (uint16_t i = 0; i < 9; ++i)
        g_rt->mem.write(static_cast<uint16_t>(destination + i), 0xcc);

    REQUIRE(g_rt->call16(rt_sym::gettimeofday, destination, 0));
    REQUIRE_EQ(g_rt->snap().de, 0);

    // (2921 days * 86400) + 12:34:56 = 0x0f0b9e70.
    static constexpr std::array<uint8_t, 8> expected = {
        0x70, 0x9e, 0x0b, 0x0f, 0x00, 0x00, 0x00, 0x00
    };
    for (std::size_t i = 0; i < expected.size(); ++i) {
        REQUIRE_EQ(g_rt->mem.read(
                       static_cast<uint16_t>(destination + i)),
                   expected[i]);
    }
    REQUIRE_EQ(g_rt->mem.read(destination + 8), 0xcc);

    for (std::size_t i = 0; i < saved.size(); ++i)
        g_rt->mem.write(static_cast<uint16_t>(5 + i), saved[i]);
}
