//
// Execute a linked Z80 test image inside the bundled header-only emulator.
// The runner understands raw binaries and Intel HEX images, watches a fixed
// completion mailbox in emulated memory, and reports the 16-bit return code
// written there by the test CRT startup stub.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "z80.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct options {
    std::string image_path;
    std::string format = "auto";
    uint16_t    start_pc = 0x0000;
    uint16_t    done_addr = 0xff02;
    uint16_t    result_addr = 0xff00;
    uint8_t     done_magic = 0xa5;
    int         cycle_budget = 2000000;
};

struct machine {
    uint8_t mem[65536]{};
    Z80     cpu;

    bool     done = false;
    uint16_t done_addr = 0xff02;
    uint8_t  done_magic = 0xa5;

    machine() {
        cpu.setupCallback(
            [](void *arg, unsigned short addr) -> unsigned char {
                return static_cast<machine *>(arg)->mem[addr];
            },
            [](void *arg, unsigned short addr, unsigned char value) {
                auto *m = static_cast<machine *>(arg);
                m->mem[addr] = value;
                if (addr == m->done_addr && value == m->done_magic) {
                    m->done = true;
                    m->cpu.requestBreak();
                }
            },
            [](void *, unsigned short) -> unsigned char { return 0xff; },
            [](void *, unsigned short, unsigned char) {},
            this);
    }
};

[[noreturn]] void die(const char *msg) {
    std::fprintf(stderr, "z80_exec: error: %s\n", msg);
    std::exit(3);
}

[[noreturn]] void dief(const char *fmt, const std::string &arg) {
    std::fprintf(stderr, "z80_exec: error: ");
    std::fprintf(stderr, fmt, arg.c_str());
    std::fprintf(stderr, "\n");
    std::exit(3);
}

int hex_nibble(char ch) {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return 10 + (ch - 'a');
    if (ch >= 'A' && ch <= 'F') return 10 + (ch - 'A');
    throw std::runtime_error("bad hex digit");
}

uint8_t parse_hex_byte(const std::string &line, size_t off) {
    return static_cast<uint8_t>((hex_nibble(line.at(off)) << 4) |
                                hex_nibble(line.at(off + 1)));
}

bool ends_with(const std::string &s, const char *suffix) {
    const std::string tail = suffix;
    if (tail.size() > s.size()) return false;
    return s.compare(s.size() - tail.size(), tail.size(), tail) == 0;
}

void load_binary(machine &m, const std::string &path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) dief("cannot open '%s'", path);

    std::vector<char> bytes((std::istreambuf_iterator<char>(in)),
                            std::istreambuf_iterator<char>());
    if (bytes.size() > sizeof(m.mem))
        die("binary image is larger than 64K");

    for (size_t i = 0; i < bytes.size(); ++i)
        m.mem[i] = static_cast<uint8_t>(bytes[i]);
}

void load_ihx(machine &m, const std::string &path) {
    std::ifstream in(path);
    if (!in) dief("cannot open '%s'", path);

    std::string line;
    uint32_t ext_base = 0;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        if (line[0] != ':')
            dief("'%s' is not a valid Intel HEX file", path);
        if (line.size() < 11)
            die("truncated Intel HEX record");

        const uint8_t len = parse_hex_byte(line, 1);
        const uint16_t addr = static_cast<uint16_t>((parse_hex_byte(line, 3) << 8) |
                                                    parse_hex_byte(line, 5));
        const uint8_t type = parse_hex_byte(line, 7);

        if (type == 0x00) {
            for (uint8_t i = 0; i < len; ++i) {
                const uint32_t full = ext_base + addr + i;
                if (full >= 65536u)
                    die("Intel HEX record writes beyond 64K");
                m.mem[full] = parse_hex_byte(line, 9 + i * 2);
            }
        } else if (type == 0x01) {
            return;
        } else if (type == 0x04) {
            ext_base = static_cast<uint32_t>(((parse_hex_byte(line, 9) << 8) |
                                              parse_hex_byte(line, 11)) << 16);
        }
    }
}

uint16_t parse_u16(const std::string &s) {
    char *end = nullptr;
    const unsigned long v = std::strtoul(s.c_str(), &end, 0);
    if (!end || *end != '\0' || v > 0xfffful)
        dief("bad 16-bit value '%s'", s);
    return static_cast<uint16_t>(v);
}

uint8_t parse_u8(const std::string &s) {
    char *end = nullptr;
    const unsigned long v = std::strtoul(s.c_str(), &end, 0);
    if (!end || *end != '\0' || v > 0xfful)
        dief("bad 8-bit value '%s'", s);
    return static_cast<uint8_t>(v);
}

int parse_int(const std::string &s) {
    char *end = nullptr;
    const long v = std::strtol(s.c_str(), &end, 0);
    if (!end || *end != '\0')
        dief("bad integer value '%s'", s);
    return static_cast<int>(v);
}

options parse_args(int argc, char **argv) {
    options opts;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto need_value = [&](const char *name) -> std::string {
            if (i + 1 >= argc) {
                std::fprintf(stderr,
                             "z80_exec: error: %s requires a value\n", name);
                std::exit(3);
            }
            return argv[++i];
        };

        if (arg == "--ihx") {
            opts.format = "ihx";
        } else if (arg == "--bin") {
            opts.format = "bin";
        } else if (arg == "--start-pc") {
            opts.start_pc = parse_u16(need_value("--start-pc"));
        } else if (arg == "--done-addr") {
            opts.done_addr = parse_u16(need_value("--done-addr"));
        } else if (arg == "--result-addr") {
            opts.result_addr = parse_u16(need_value("--result-addr"));
        } else if (arg == "--done-magic") {
            opts.done_magic = parse_u8(need_value("--done-magic"));
        } else if (arg == "--cycles") {
            opts.cycle_budget = parse_int(need_value("--cycles"));
        } else if (!arg.empty() && arg[0] == '-') {
            dief("unknown option '%s'", arg);
        } else if (opts.image_path.empty()) {
            opts.image_path = arg;
        } else {
            die("too many positional arguments");
        }
    }

    if (opts.image_path.empty()) {
        std::fprintf(stderr,
                     "Usage: z80_exec [--ihx|--bin] [--cycles N] image\n");
        std::exit(3);
    }

    if (opts.format == "auto") {
        if (ends_with(opts.image_path, ".ihx"))
            opts.format = "ihx";
        else
            opts.format = "bin";
    }

    return opts;
}

} // namespace

int main(int argc, char **argv) {
    const options opts = parse_args(argc, argv);

    machine m;
    m.done_addr = opts.done_addr;
    m.done_magic = opts.done_magic;

    if (opts.format == "ihx")
        load_ihx(m, opts.image_path);
    else if (opts.format == "bin")
        load_binary(m, opts.image_path);
    else
        die("internal bad format");

    m.cpu.reg.PC = opts.start_pc;
    const int executed = m.cpu.execute(opts.cycle_budget);
    const uint16_t result = static_cast<uint16_t>(
        m.mem[opts.result_addr] |
        (static_cast<uint16_t>(m.mem[(opts.result_addr + 1) & 0xffff]) << 8));

    std::printf("done=%d return=%u cycles=%d pc=0x%04x\n",
                m.done ? 1 : 0, result, executed, m.cpu.reg.PC);

    if (!m.done)
        return 2;
    return 0;
}
