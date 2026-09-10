#include <xz80/xz80.h>
#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
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
    std::ifstream cases(argv[2]);
    unsigned address, kind, checks = 0;
    while (cases >> address >> kind) {
        for (unsigned input = 0; input < 256; ++input) {
            for (unsigned count = 0; count < 8; ++count) {
                unsigned expected;
                if (kind == 0) expected = (input << count) & 255;
                else if (kind == 1) expected = input >> count;
                else {
                    // Explicit floor division avoids relying on host signed right shift.
                    int signed_input = input < 128 ? int(input) : int(input) - 256;
                    const int divisor = 1 << count;
                    int quotient = signed_input / divisor;
                    if (signed_input < 0 && signed_input % divisor) --quotient;
                    expected = static_cast<unsigned>(quotient) & 255;
                }
                const unsigned argument = input | (count << 8);
                mem.word(0xfe00, 0xff00); mem.word(0xfe02, argument);
                cpu.reset(); auto state = cpu.snapshot();
                state.pc = address; state.sp = 0xfe00; state.hl = argument;
                state.de = 0xabcd; state.ix = 0x9876; state.iy = 0x4567;
                cpu.restore(state);
                unsigned steps = 0;
                while (cpu.pc() != 0xff00 && steps++ < 10000) cpu.step();
                state = cpu.snapshot();
                unsigned actual = abi ? state.de : state.hl;
                if (cpu.pc() != 0xff00 || actual != expected) {
                    std::cerr << "kind=" << kind << " input=" << input << " count=" << count
                              << " expected=" << expected << " actual=" << actual
                              << " pc=" << cpu.pc() << '\n';
                    return 1;
                }
                ++checks;
            }
        }
    }
    std::cout << checks << " exhaustive byte-shift checks passed\n";
}
