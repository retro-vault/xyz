// Execute generated code with observable memory and port reads.
#include <xz80/xz80.h>
#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
struct memory final : xz80::IMemory {
    std::array<uint8_t, 65536> data{};
    mutable std::array<unsigned, 2> reads{};
    std::array<unsigned, 2> writes{};
    uint16_t watch = 0xc000;
    bool changing = false;
    bool observe = true;
    bool trace_access = false;
    mutable std::array<char, 256> trace{};
    mutable unsigned trace_size = 0;
    uint8_t read(uint16_t address) const noexcept override {
        if (observe && (address == watch || address == uint16_t(watch + 1))) {
            unsigned byte = uint16_t(address - watch);
            unsigned before = reads[byte]++;
            if (trace_access && trace_size < trace.size())
                trace[trace_size++] = byte ? 'R' : 'r';
            if (changing && byte == 0) return uint8_t(data[address] + before);
        }
        return data[address];
    }
    void write(uint16_t address, uint8_t value) noexcept override {
        if (observe && (address == watch || address == uint16_t(watch + 1))) {
            ++writes[uint16_t(address - watch)];
            if (trace_access && trace_size < trace.size())
                trace[trace_size++] = address == watch ? 'w' : 'W';
        }
        data[address] = value;
    }
    void put16(uint16_t address, uint16_t value) {
        data[address] = uint8_t(value); data[uint16_t(address+1)] = uint8_t(value>>8);
    }
    uint16_t get16(uint16_t address) const {
        return data[address] | (unsigned(data[uint16_t(address+1)]) << 8);
    }
};
struct ports final : xz80::IPorts {
    unsigned reads = 0;
    uint8_t value = 0;
    uint8_t in(uint16_t) noexcept override { return uint8_t(value + reads++); }
    void out(uint16_t, uint8_t) noexcept override {}
};
int main(int argc, char **argv) {
    if (argc != 8) return 2;
    const std::string mode = argv[2];
    const unsigned which = std::stoul(argv[3]), abi = std::stoul(argv[4]);
    const uint16_t entry = std::stoul(argv[5]), object = std::stoul(argv[6]);
    const uint16_t calls = std::stoul(argv[7]);
    memory mem; ports io; xz80::cpu cpu(mem, io);
    std::ifstream input(argv[1], std::ios::binary);
    if (!input) return 2;
    input.read(reinterpret_cast<char *>(mem.data.data()), mem.data.size());
    unsigned checks = 0;
    auto execute = [&](uint16_t argument, uint16_t argument2) {
        mem.put16(0xfe00, 0xff00); mem.put16(0xfe02, argument);
        mem.put16(0xfe04, argument2);
        cpu.reset(); auto state = cpu.snapshot();
        state.pc = entry; state.sp = 0xfe00;
        state.hl = argument; state.de = argument2;
        if (mode == "byte_cache")
            state.af = uint16_t(argument << 8);
        state.ix = 0x9876; state.iy = 0x4567;
        cpu.restore(state); mem.reads = {}; mem.writes = {}; io.reads = 0;
        mem.trace_size = 0;
        unsigned steps = 0;
        const unsigned instruction_limit = mode == "memory" ? 1000000 :
                                           mode == "rmw_array" ? 200000 :
                                           mode == "volatile_loop" ? 100000 :
                                           mode == "address" ? 100000 : 2000;
        while (cpu.pc() != 0xff00 && ++steps < instruction_limit) {
            // A shared prologue temporarily puts its return address where a
            // local will subsequently live.  Those stack operations are not
            // accesses to the local object whose lifetime starts afterwards.
            unsigned opcode = mem.data[cpu.pc()];
            if (opcode == 0xdd || opcode == 0xfd)
                opcode = mem.data[uint16_t(cpu.pc()+1)];
            mem.observe = mode == "indirect" ||
                !((opcode & 0xcf) == 0xc1 || (opcode & 0xcf) == 0xc5 ||
                  opcode == 0xc9 || opcode == 0xcd || opcode == 0xe3);
            cpu.step();
        }
        if (cpu.pc() != 0xff00) throw std::runtime_error("instruction limit");
        if (mode == "byte_cache")
            return uint16_t(abi ? cpu.snapshot().af >> 8 : cpu.snapshot().hl & 255);
        return abi ? cpu.snapshot().de : cpu.snapshot().hl;
    };
    auto require = [&](bool ok, const std::string &message, unsigned value) {
        if (!ok) throw std::runtime_error(message + " input=" + std::to_string(value) +
            " reads=" + std::to_string(mem.reads[0]) + "," + std::to_string(mem.reads[1]) +
            " writes=" + std::to_string(mem.writes[0]) + "," + std::to_string(mem.writes[1]));
        ++checks;
    };
    if (mode == "address" || mode == "memory" || mode == "argument") {
        mem.watch = mode != "address" ? object : 0;
        for (unsigned value : {0u, 1u, 2u, 127u, 255u, 256u, 32767u, 65535u}) {
            require(execute(value, 0) == 0, "read/modify/write address", value);
            if ((mode == "memory" || mode == "argument") && object != 0)
                require(mem.reads[0] == 1 && mem.reads[1] == 1 &&
                        mem.writes[0] == 1 && mem.writes[1] == 1,
                        "zero-count observable argument", value);
            if (mode == "argument" && object == 0)
                require(io.reads == 1, "observable port argument", value);
        }
    } else if (mode == "switch") {
        for (unsigned value : {0u, 1u, 2u, 3u, 255u, 256u, 65535u}) {
            mem.watch = which == 2 ? 0xfdfc : object;
            mem.put16(object, value); mem.put16(calls, 0);
            mem.changing = true; io.value = uint8_t(value);
            unsigned sampled = which == 4 ? 2 : which == 5 ? uint8_t(value) : value;
            unsigned expected = which == 3 || which == 6 || which == 9 ?
                44 : sampled < 3 ? 41+sampled : 44;
            unsigned result = execute(value, 0);
            require(result == expected, "switch result", value);
            if (which == 5) require(io.reads == 1, "SFR read count", value);
            else if (which == 4 || which == 6) require(mem.get16(calls) == 1, "call count", value);
            else require(mem.reads[0] == 1 && mem.reads[1] == 1, "switch read count", value);
        }
    } else if (mode == "local") {
        for (unsigned value : {0u, 1u, 2u, 127u, 254u, 255u, 256u, 65535u}) {
            const bool byte = which == 2 || which == 5 || which == 9;
            mem.watch = byte ? 0xfdfd : 0xfdfc;
            unsigned result = execute(value, 0);
            unsigned expected = which == 2 ? (uint8_t(value+1) ? 9 : 7) :
                which == 4 || which == 8 ? value :
                which == 5 || which == 9 ? uint8_t(value) :
                which == 6 || which == 7 ? uint16_t(value+1) : value != 0;
            require(result == expected, "increment result", value);
            require(mem.reads[0] == (which == 5 ? 0u : 1u) &&
                mem.writes[0] == (which == 5 ? 1u : 2u), "low byte accesses", value);
            if (!byte)
                require(mem.reads[1] == 1 && mem.writes[1] == 2, "high byte accesses", value);
        }
    } else if (mode == "byte_cache") {
        mem.watch = 0xfdfd; // First byte local, after the saved IX.
        mem.changing = true;
        mem.trace_access = true;
        for (unsigned value = 0; value < 256; ++value) {
            const unsigned shifted = which == 2 ? value : uint8_t(value + 1);
            const unsigned expected = uint8_t((shifted << 1) ^ ((value & 128) ? 83 : 0));
            require(execute(value, 0) == expected, "typed byte sampled values", value);
            require(mem.reads[0] == (which == 2 ? 1u : 2u) && mem.reads[1] == 0,
                    "typed byte exact reads", value);
            require(mem.writes[0] == 1 && mem.writes[1] == 0,
                    "typed byte exact writes", value);
            require(std::string(mem.trace.data(), mem.trace_size) ==
                    (which == 2 ? "wr" : "wrr"), "typed byte ordered accesses", value);
        }
    } else if (mode == "volatile_loop") {
        mem.watch = 0xfdfc; // First word local, after the saved IX.
        mem.changing = true;
        mem.trace_access = true;
        for (unsigned count : {0u, 1u, 2u, 3u, 7u, 17u}) {
            // Each word read has a distinct low byte: iteration i sees
            // 5*i, 5*i+1, 5*i+2, 5*i+3, all below 256 in this probe.
            const unsigned expected = 10u * count * (count - 1u) + 6u * count;
            require(execute(count, 0) == expected, "volatile loop sampled values", count);
            require(mem.reads[0] == 4*count && mem.reads[1] == 4*count,
                    "volatile loop read count", count);
            require(mem.writes[0] == count+1 && mem.writes[1] == count+1,
                    "volatile loop write count", count);
            std::string expected_trace = "wW";
            for (unsigned i = 0; i < count; ++i)
                expected_trace += "wWrRrRrRrR";
            require(std::string(mem.trace.data(), mem.trace_size) == expected_trace,
                    "volatile loop ordered accesses", count);
        }
    } else if (mode == "rmw_array") {
        mem.watch = object;
        const unsigned rmw_case = which % 100;
        for (unsigned value = 0; value < 65536; value += which >= 100 ? 1 : rmw_case == 6 ? 8191 : 257) {
            for (unsigned addend : {0u, 255u, 32769u, 65535u}) {
                uint16_t checksum = 0;
                std::array<uint16_t, 19> expected{};
                for (unsigned i = 0; i < expected.size(); ++i) {
                    uint16_t before = uint16_t(value + i * 257u);
                    uint16_t from = uint16_t(addend + i * 977u);
                    mem.put16(uint16_t(object + 2*i), before);
                    mem.put16(uint16_t(calls + 2*i), from);
                    expected[i] = uint16_t(rmw_case == 3 ? before * 2u :
                        rmw_case == 4 ? before * 8u : before + from * 7u);
                    if (rmw_case == 5 || (rmw_case == 8 && i < 18))
                        checksum += expected[i];
                    else if (rmw_case == 7 && i < 18)
                        checksum += before;
                }
                unsigned result = execute(0, 0);
                require(result == checksum, "RMW return", value);
                for (unsigned i = 0; i < expected.size(); ++i) {
                    require(mem.get16(uint16_t(object + 2*i)) == expected[i],
                            "RMW value", value);
                    require(mem.get16(uint16_t(calls + 2*i)) ==
                            uint16_t(addend + i * 977u), "RMW source intact", value);
                }
                if (rmw_case == 2)
                    require(mem.reads[0] == 1 && mem.reads[1] == 1 &&
                            mem.writes[0] == 1 && mem.writes[1] == 1,
                            "volatile RMW exact accesses", value);
            }
        }
    } else if (mode == "indirect") {
        for (uint16_t address : {uint16_t(0xc000), uint16_t(0xc001), uint16_t(0xffff)}) {
            mem.watch = address;
            for (unsigned value = 0; value < 65536; ++value) {
                mem.put16(address, value);
                mem.data[uint16_t(address-1)] = 0xa5;
                mem.data[uint16_t(address+2)] = 0x5a;
                unsigned result = execute(address, address);
                unsigned delta = which == 5 ? 2 : 1;
                require(mem.get16(address) == uint16_t(value+delta), "indirect result", value);
                if (which == 3) require(result == value, "postincrement result", value);
                if (which == 4) require(mem.reads[0] == 1 && mem.reads[1] == 1 &&
                    mem.writes[0] == 1 && mem.writes[1] == 1, "volatile indirect accesses", value);
                require(mem.data[uint16_t(address-1)] == 0xa5 &&
                    mem.data[uint16_t(address+2)] == 0x5a, "adjacent bytes", value);
            }
        }
    } else return 2;
    std::cout << checks << " checks passed\n";
}
