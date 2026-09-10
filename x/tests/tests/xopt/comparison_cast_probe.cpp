// Exhaustively execute comparison casts and unrelated truncation boundaries.
#include <xz80/xz80.h>
#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>

struct memory final : xz80::IMemory {
    std::array<uint8_t, 65536> data{};
    uint8_t read(uint16_t address) const noexcept override { return data[address]; }
    void write(uint16_t address, uint8_t value) noexcept override { data[address] = value; }
    void word(uint16_t address, uint16_t value) {
        data[address] = uint8_t(value);
        data[uint16_t(address + 1)] = uint8_t(value >> 8);
    }
};
struct ports final : xz80::IPorts {
    uint8_t in(uint16_t) noexcept override { return 0; }
    void out(uint16_t, uint8_t) noexcept override {}
};
int main(int argc, char **argv) {
    if (argc != 4) return 2;
    const unsigned abi = std::stoul(argv[3]);
    memory mem;
    ports io;
    xz80::cpu cpu(mem, io);
    std::ifstream binary(argv[1], std::ios::binary);
    binary.read(reinterpret_cast<char *>(mem.data.data()), mem.data.size());
    if (binary.gcount() != 65536) return 2;
    std::ifstream cases(argv[2]);
    unsigned entry, operation, bytes, kind, checks = 0;
    while (cases >> entry >> operation >> bytes >> kind) {
        for (unsigned value = 0; value < 65536; ++value) {
            const unsigned a = value & 255, b = value >> 8;
            unsigned expected = 0;
            if (kind == 1) {
                switch (operation) {
                case 0: expected = a; break;
                case 1: expected = a ? 17 : 23; break;
                case 2: expected = value & 1; break;
                case 3: expected = (value & 1) ? 17 : 23; break;
                }
            } else if (kind == 2) {
                expected = a == b ? 255 : 0;
            } else {
                switch (operation) {
                case 0: expected = a == b; break;
                case 1: expected = a != b; break;
                case 2: expected = a < b; break;
                case 3: expected = a <= b; break;
                case 4: expected = a > b; break;
                case 5: expected = a >= b; break;
                }
            }
            mem.word(0xfe00, 0xff00);
            mem.word(0xfe02, value);
            cpu.reset();
            auto state = cpu.snapshot();
            state.pc = uint16_t(entry);
            state.sp = 0xfe00;
            state.af = uint16_t(a << 8);
            state.hl = uint16_t(kind == 1 ? value : b);
            state.de = 0x2468;
            state.ix = 0x9876;
            state.iy = 0x4567;
            cpu.restore(state);
            unsigned steps = 0;
            while (cpu.pc() != 0xff00 && ++steps < 2000) cpu.step();
            state = cpu.snapshot();
            const unsigned actual = bytes == 1
                ? (abi ? state.af >> 8 : state.hl & 255)
                : (abi ? state.de : state.hl);
            if (cpu.pc() != 0xff00 || actual != expected ||
                state.sp != 0xfe02 || state.ix != 0x9876 || state.iy != 0x4567) {
                std::cerr << "entry=" << entry << " kind=" << kind
                          << " input=" << value << " expected=" << expected
                          << " actual=" << actual << " PC=" << state.pc
                          << " SP=" << state.sp << '\n';
                return 1;
            }
            ++checks;
        }
    }
    std::cout << checks << " exhaustive comparison-cast checks passed\n";
}
