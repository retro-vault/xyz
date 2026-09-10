// Execute original and optimized machine code with the declared return mask.
#include <xz80/xz80.h>
#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

struct memory final : xz80::IMemory {
    std::array<uint8_t, 65536> data{};
    uint8_t read(uint16_t address) const noexcept override { return data[address]; }
    void write(uint16_t address, uint8_t value) noexcept override { data[address] = value; }
};
struct ports final : xz80::IPorts {
    uint8_t in(uint16_t) noexcept override { return 0; }
    void out(uint16_t, uint8_t) noexcept override {}
};

int main(int argc, char **argv) {
    if (argc != 3) return 2;
    memory initial; ports io;
    std::ifstream input(argv[1], std::ios::binary);
    input.read(reinterpret_cast<char *>(initial.data.data()), initial.data.size());
    if (input.gcount() != 65536) return 2;
    std::ifstream cases(argv[2]);
    unsigned original, optimized, mask, checks = 0;
    while (cases >> original >> optimized >> mask) {
        for (unsigned value = 0; value < 256; ++value) {
            for (unsigned flags : {0u, 1u, 0x40u, 0x45u, 0x80u, 0xffu}) {
                auto run = [&](unsigned pc) {
                    memory mem = initial; xz80::cpu cpu(mem, io);
                    auto state = cpu.snapshot();
                    state.pc = pc; state.sp = 0xfe00;
                    state.ix = 0xc000; state.iy = 0xa55a;
                    state.af = (value << 8) | flags;
                    state.bc = (value << 8) | (value ^ 0xa5);
                    state.de = (value * 257) ^ 0x1b4d;
                    state.hl = (value * 257) ^ 0x9c37;
                    state.de2 = (value * 257) ^ 0x7621;
                    state.hl2 = (value * 257) ^ 0xcd89;
                    mem.data[0xfe00] = 0; mem.data[0xfe01] = 0xff;
                    cpu.restore(state);
                    unsigned steps = 0;
                    while (cpu.pc() != 0xff00 && ++steps < 1000) cpu.step();
                    if (cpu.pc() != 0xff00) throw std::runtime_error("return fixture did not terminate");
                    return cpu.snapshot();
                };
                const auto before = run(original), after = run(optimized);
                auto bytes = [](const auto &state) {
                    const unsigned af = state.af, bc = state.bc, de = state.de, hl = state.hl;
                    const unsigned de2 = state.de2, hl2 = state.hl2;
                    return std::array<unsigned, 11>{af >> 8, bc >> 8, bc & 255,
                        de >> 8, de & 255, hl >> 8, hl & 255,
                        de2 >> 8, de2 & 255, hl2 >> 8, hl2 & 255};
                };
                const auto expected = bytes(before), actual = bytes(after);
                for (unsigned bit = 0; bit < actual.size(); ++bit) {
                    if ((mask & (1u << bit)) && expected[bit] != actual[bit]) {
                        std::cerr << "return register mismatch at " << original << '/' << optimized
                                  << " mask " << mask << " byte " << bit << " input " << value
                                  << " expected " << expected[bit] << " actual " << actual[bit] << '\n';
                        return 1;
                    }
                }
                if (before.sp != after.sp || before.ix != after.ix || before.iy != after.iy)
                    throw std::runtime_error("return fixture changed preserved state");
                ++checks;
            }
        }
    }
    std::cout << checks << " return-register machine comparisons passed\n";
}
