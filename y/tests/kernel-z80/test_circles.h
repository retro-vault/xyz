// Midpoint-circle pixel regressions for shared outline/fill arithmetic.
// GPL-2.0 License (see: ../../src/z80/gpx/LICENSE.libgpx)
// Copyright (C) 2026 Tomaz Stih
#ifndef YOS_TEST_CIRCLES_H
#define YOS_TEST_CIRCLES_H

#include <algorithm>
#include <array>
#include <cstdint>
#include <stdexcept>
#include <utility>

template<class Memory, class Call, class Symbol>
void test_circles(Memory& mem, Call call, Symbol sym, std::uint16_t context)
{
    constexpr std::uint16_t pattern = 0xe180, clip = 0xe182;
    mem.bytes[pattern] = 0xff;
    mem.word(clip, 5); mem.word(clip + 2, 4);
    mem.word(clip + 4, 250); mem.word(clip + 6, 187);
    for (const auto [cx, cy] : {std::pair{128, 96}, std::pair{0, 0},
                               std::pair{255, 191}, std::pair{-5, 80}}) {
        for (int radius : {-1, 0, 1, 2, 3, 4, 5, 7, 8, 15, 31}) {
            for (bool filled : {false, true}) {
                for (bool clipped : {false, true}) {
                    std::array<std::uint8_t, 6144> expected{};
                    const auto pixel = [&](int x, int y) {
                        if (x < 0 || x > 255 || y < 0 || y > 191 ||
                            (clipped && (x < 5 || x > 250 || y < 4 || y > 187)))
                            return;
                        const auto offset = ((y & 0xc0) << 5) |
                            ((y & 7) << 8) | ((y & 0x38) << 2) | (x >> 3);
                        expected[offset] |= 0x80 >> (x & 7);
                    };
                    const auto row = [&](int dy, int width) {
                        for (int dx = -width; dx <= width; ++dx)
                            pixel(cx + dx, cy + dy);
                    };
                    const auto pair = [&](int dy, int width) {
                        row(dy, width); row(-dy, width);
                    };
                    if (radius == 0) pixel(cx, cy);
                    if (radius > 0) {
                        if (filled) row(0, radius);
                        else {
                            pixel(cx, cy + radius); pixel(cx, cy - radius);
                            pixel(cx + radius, cy); pixel(cx - radius, cy);
                        }
                        int x = 0, y = radius, decision = 1 - radius;
                        do {
                            ++x;
                            decision += 2 * x + 1;
                            if (decision >= 0) {
                                if (filled) pair(y, x - 1);
                                --y;
                                decision -= 2 * y;
                            }
                            if (x > y) break;
                            if (filled) pair(x, y);
                            else for (int sx : {-1, 1}) for (int sy : {-1, 1}) {
                                pixel(cx + sx * x, cy + sy * y);
                                pixel(cx + sx * y, cy + sy * x);
                            }
                        } while (x < y);
                    }
                    const auto byte = [](int n) { return std::uint8_t(n); };
                    const auto clipping = clipped ? clip : 0;
                    for (unsigned mode : {0u, 1u, 2u}) {
                        std::fill_n(mem.bytes.begin() + 0x4000, 6144, 0);
                        const auto draw = [&] {
                            if (filled)
                                call(sym("_gpx_fill_circle"), context, cx,
                                     "filled circle pixels", 0,
                                     {byte(cy), byte(cy >> 8), byte(radius),
                                      byte(radius >> 8), 1, byte(mode),
                                      byte(pattern), byte(pattern >> 8), 1,
                                      byte(clipping), byte(clipping >> 8)});
                            else
                                call(sym("_gpx_draw_circle"), context, cx,
                                     "outline circle pixels", 0,
                                     {byte(cy), byte(cy >> 8), byte(radius),
                                      byte(radius >> 8), 1, byte(mode),
                                      byte(clipping), byte(clipping >> 8)});
                        };
                        draw();
                        if (!std::equal(expected.begin(), expected.end(),
                                        mem.bytes.begin() + 0x4000))
                            throw std::runtime_error("circle pixel mismatch");
                        if (mode == 1) {
                            draw();
                            if (!std::all_of(mem.bytes.begin() + 0x4000,
                                             mem.bytes.begin() + 0x5800,
                                             [](auto b) { return b == 0; }))
                                throw std::runtime_error("circle XOR erase");
                        }
                    }
                }
            }
        }
    }
}
#endif
