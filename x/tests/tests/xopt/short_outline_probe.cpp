// Check repeated indexed loads through real CALL/RET execution.
#include <xz80/xz80.h>
#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

struct memory final : xz80::IMemory {
    std::array<uint8_t, 65536> data{};
    mutable std::vector<uint16_t> reads;
    uint16_t watched = 0xc000;
    uint8_t read(uint16_t address) const noexcept override {
        if (address == watched || address == uint16_t(watched + 1)) reads.push_back(address);
        return data[address];
    }
    void write(uint16_t address, uint8_t value) noexcept override { data[address] = value; }
    void word(uint16_t address, uint16_t value) {
        data[address] = value; data[uint16_t(address + 1)] = value >> 8;
    }
};
struct ports final : xz80::IPorts {
    uint8_t in(uint16_t) noexcept override { return 0; }
    void out(uint16_t, uint8_t) noexcept override {}
};

int main(int argc, char **argv) {
    if (argc < 5) return 2;
    memory mem; ports io; xz80::cpu cpu(mem, io);
    std::ifstream input(argv[1], std::ios::binary);
    if (!input) return 2;
    input.read(reinterpret_cast<char *>(mem.data.data()), mem.data.size());
    const unsigned entry = std::stoul(argv[2]);
    const bool framed = std::string(argv[3]) == "--frame";
    if (framed) mem.watched = 0xfdfc;
    std::vector<unsigned> consumers;
    for (int i = framed ? 4 : 3; i < argc; ++i)
        consumers.push_back(std::stoul(argv[i]));
    unsigned checks = 0;
    auto require = [&](bool condition) {
        if (!condition) throw std::runtime_error("outlined load changed observable state");
        ++checks;
    };
    for (unsigned flags = 0; flags < 256; ++flags) {
        for (unsigned value : {0u, 255u, 256u, 32767u, 65535u}) {
            cpu.reset(); auto state = cpu.snapshot();
            state.pc = entry; state.sp = 0xfe00; state.ix = 0xc000;
            state.iy = 0xa55a; state.bc = 0x1357;
            state.de = framed ? value : 0x2468;
            state.af = 0x5a00 | flags;
            cpu.restore(state); mem.word(0xfe00, 0xff00);
            mem.word(mem.watched, value); mem.reads.clear();
            unsigned consumed = 0, steps = 0;
            unsigned expected_af = 0x5a00 | flags;
            while (cpu.pc() != 0xff00 && ++steps < 200) {
                if (consumed < consumers.size() && cpu.pc() == consumers[consumed]) {
                    state = cpu.snapshot();
                    if (framed && consumed == 0) expected_af = state.af;
                    require(state.hl == uint16_t(value + consumed * 257));
                    require(state.af == expected_af);
                    require(mem.reads == std::vector<uint16_t>(
                        {mem.watched, uint16_t(mem.watched + 1)}));
                    mem.reads.clear(); ++consumed;
                    // Each unknown call changes the object and destroys HL.
                    // Forwarding a prior load across it must remain forbidden.
                    mem.word(mem.watched, value + consumed * 257);
                    state.hl = 0xdead; cpu.restore(state);
                }
                cpu.step();
            }
            state = cpu.snapshot();
            require(state.pc == 0xff00 && consumed == consumers.size());
            require(state.sp == 0xfe02 && state.ix == 0xc000 && state.iy == 0xa55a);
            require(state.bc == 0x1357 && state.de == (framed ? value : 0x2468));
        }
    }
    std::cout << checks << " short-outline execution checks passed\n";
}
