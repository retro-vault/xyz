#include <xz80/xz80.h>
#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>

struct memory final : xz80::IMemory {
    std::array<uint8_t, 65536> data{};
    uint8_t read(uint16_t address) const noexcept override { return data[address]; }
    void write(uint16_t address, uint8_t value) noexcept override { data[address] = value; }
    void word(uint16_t address, uint16_t value) { data[address] = value; data[uint16_t(address + 1)] = value >> 8; }
};
struct ports final : xz80::IPorts {
    uint8_t in(uint16_t) noexcept override { return 0; }
    void out(uint16_t, uint8_t) noexcept override {}
};
int main(int argc, char **argv) {
    if (argc != 4) return 2;
    const unsigned abi = std::stoul(argv[3]);
    memory mem; ports io; xz80::cpu cpu(mem, io);
    std::ifstream image(argv[1], std::ios::binary);
    image.read(reinterpret_cast<char *>(mem.data.data()), mem.data.size());
    if (image.gcount() != 65536) return 2;
    std::ifstream cases(argv[2]);
    std::string name;
    unsigned address, family, source_sign, width, sign, wide, checks = 0;
    while (cases >> name >> address >> family >> source_sign >> width >> sign >> wide) {
        for (unsigned input = 0; input < 256; ++input) {
            // C casts keep low precision bits and then sign-extend a signed
            // narrow value. Compute in wide host integers, independently of
            // the target's casts, promotion, memory, and register allocation.
            const unsigned raw = family == 4 ? input & 0xdf : input;
            const unsigned mask = (1u << width) - 1;
            int64_t narrow = raw & mask;
            if (sign && (unsigned(narrow) & (1u << (width - 1)))) narrow -= int64_t{1} << width;
            if (family == 1) narrow += 0xc001;
            const uint64_t wide_mask = (uint64_t{1} << wide) - 1;
            const uint32_t expected = uint64_t(narrow) & wide_mask;
            mem.data[0xc000] = input;
            mem.word(0xfe00, 0xff00);
            mem.word(0xfe02, family < 2 ? 0xc000 : input);
            cpu.reset(); auto state = cpu.snapshot();
            state.pc = address; state.sp = 0xfe00;
            state.hl = family < 2 ? 0xc000 : 0x5abc;
            state.af = (input << 8) | 0x45;
            state.de = 0xabcd; state.bc = 0x1234;
            state.ix = 0x9876; state.iy = 0x4567;
            cpu.restore(state);
            unsigned steps = 0;
            while (cpu.pc() != 0xff00 && steps++ < 10000) cpu.step();
            state = cpu.snapshot();
            uint32_t actual = abi ? state.de : state.hl;
            if (wide == 32) actual |= uint32_t(abi ? state.hl : state.de) << 16;
            if (cpu.pc() != 0xff00 || state.ix != 0x9876 || state.sp != 0xfe02 || actual != expected) {
                std::cerr << name << " input=" << input << " expected=" << expected
                          << " actual=" << actual << " pc=" << cpu.pc()
                          << " sp=" << state.sp << " ix=" << state.ix << '\n';
                return 1;
            }
            ++checks;
        }
    }
    std::cout << checks << " byte numeric-view checks passed\n";
}
