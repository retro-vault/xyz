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
    unsigned address, source_width, product_width, width, sign, shift, factor, checks = 0;
    const unsigned inputs[] = {0,1,127,128,255,256,511,512,1023,32767,32768,65535};
    while (cases >> address >> source_width >> product_width >> width >> sign >> shift >> factor) {
        for (unsigned input_x : inputs) for (unsigned input_y : inputs) {
            if (factor && input_y != 0) continue;
            const unsigned source_mask = (1u << source_width) - 1;
            const unsigned x = input_x & source_mask, y = factor ? factor : input_y & source_mask;
            const uint64_t product_mask = (uint64_t{1} << product_width) - 1;
            const uint64_t product = (uint64_t{x} * y) & product_mask;
            const unsigned mask = (1u << width) - 1;
            unsigned expected = unsigned(product >> shift) & mask;
            if (sign && (expected & (1u << (width - 1)))) expected |= 65535u ^ mask;
            mem.word(0xfe00, 0xff00); mem.word(0xfe02, x); mem.word(0xfe04, y);
            cpu.reset(); auto state = cpu.snapshot();
            state.pc = address; state.sp = 0xfe00; state.hl = x; state.de = y;
            state.ix = 0x9876; state.iy = 0x4567; state.bc = 0x1234;
            cpu.restore(state);
            unsigned steps = 0;
            while (cpu.pc() != 0xff00 && steps++ < 10000) cpu.step();
            state = cpu.snapshot();
            unsigned actual = abi ? state.de : state.hl;
            if (cpu.pc() != 0xff00 || state.ix != 0x9876 || state.sp != 0xfe02 || actual != expected) {
                std::cerr << "source=" << source_width << " product=" << product_width
                          << " dest=" << width << " signed=" << sign << " shift=" << shift
                          << " x=" << x << " y=" << y << " expected=" << expected
                          << " actual=" << actual << " pc=" << cpu.pc() << '\n';
                return 1;
            }
            ++checks;
        }
    }
    std::cout << checks << " product precision checks passed\n";
}
