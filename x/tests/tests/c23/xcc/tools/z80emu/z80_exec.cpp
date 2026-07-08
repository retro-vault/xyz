//
// Execute a linked Z80 test image inside the bundled header-only emulator.
// The runner understands raw binaries and Intel HEX images, watches a fixed
// completion mailbox in emulated memory, and reports the 16-bit return code
// written there by the test CRT startup stub.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#define Z80_CALLBACK_PER_INSTRUCTION
#include "z80.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <fstream>
#include <iterator>
#include <memory>
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
    std::string stdin_text;
    std::string stdin_file;
    std::string stdout_file;
    std::string fs_root = ".";
    bool        print_output = false;
    bool        z88dk_trap = false;
};

constexpr uint16_t EMU_REQ_FD      = 0xff11;
constexpr uint16_t EMU_REQ_PTR     = 0xff13;
constexpr uint16_t EMU_REQ_LEN     = 0xff15;
constexpr uint16_t EMU_REQ_FLAGS   = 0xff17;
constexpr uint16_t EMU_REQ_MODE    = 0xff19;
constexpr uint16_t EMU_REQ_WHENCE  = 0xff1b;
constexpr uint16_t EMU_REQ_OFFSET  = 0xff1d;
constexpr uint16_t EMU_REQ_PATH    = 0xff21;
constexpr uint16_t EMU_REQ_PATH2   = 0xff23;
constexpr uint16_t EMU_REQ_RESULT  = 0xff25;

constexpr uint16_t EMU_PORT_CMD          = 0x00e0;
constexpr uint16_t EMU_PORT_CONOUT       = 0x00e1;
constexpr uint16_t EMU_PORT_CONIN_STATUS = 0x00e2;
constexpr uint16_t EMU_PORT_CONIN_DATA   = 0x00e3;

constexpr uint8_t EMU_CMD_EXIT   = 1;
constexpr uint8_t EMU_CMD_OPEN   = 4;
constexpr uint8_t EMU_CMD_CLOSE  = 5;
constexpr uint8_t EMU_CMD_READ   = 6;
constexpr uint8_t EMU_CMD_WRITE  = 7;
constexpr uint8_t EMU_CMD_LSEEK  = 8;
constexpr uint8_t EMU_CMD_UNLINK = 9;
constexpr uint8_t EMU_CMD_RENAME = 10;

constexpr uint16_t O_ACCMODE = 0x0003;
constexpr uint16_t O_WRONLY  = 0x0001;
constexpr uint16_t O_RDWR    = 0x0002;
constexpr uint16_t O_CREAT   = 0x0100;
constexpr uint16_t O_TRUNC   = 0x0200;
constexpr uint16_t O_APPEND  = 0x0400;

struct file_handle {
    std::fstream stream;
    uint16_t flags = 0;
};

struct machine {
    uint8_t mem[65536]{};
    Z80     cpu;

    bool     done = false;
    uint16_t done_addr = 0xff02;
    uint8_t  done_magic = 0xa5;
    uint16_t result_addr = 0xff00;
    std::deque<uint8_t> console_in;
    std::string console_out;
    std::string fs_root = ".";
    std::vector<std::unique_ptr<file_handle>> files;
    int executed_cycles = 0;

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
            [](void *arg, unsigned short port) -> unsigned char {
                return static_cast<machine *>(arg)->in_port(port);
            },
            [](void *arg, unsigned short port, unsigned char value) {
                static_cast<machine *>(arg)->out_port(port, value);
            },
            this);
    }

    uint8_t in_port(uint16_t port) {
        switch (port) {
        case EMU_PORT_CONIN_STATUS:
            return console_in.empty() ? 0 : 1;
        case EMU_PORT_CONIN_DATA: {
            if (console_in.empty())
                return 0xff;
            uint8_t ch = console_in.front();
            console_in.pop_front();
            return ch;
        }
        default:
            return 0xff;
        }
    }

    void out_port(uint16_t port, uint8_t value) {
        switch (port) {
        case EMU_PORT_CONOUT:
            console_out.push_back(static_cast<char>(value));
            break;
        case EMU_PORT_CMD:
            handle_command(value);
            break;
        default:
            break;
        }
    }

    uint8_t read8(uint16_t addr) const {
        return mem[addr];
    }

    uint16_t read16(uint16_t addr) const {
        return static_cast<uint16_t>(mem[addr] |
               (static_cast<uint16_t>(mem[(addr + 1) & 0xffff]) << 8));
    }

    uint32_t read32(uint16_t addr) const {
        return static_cast<uint32_t>(read16(addr)) |
               (static_cast<uint32_t>(read16(addr + 2)) << 16);
    }

    void write8(uint16_t addr, uint8_t value) {
        mem[addr] = value;
    }

    void write16(uint16_t addr, uint16_t value) {
        mem[addr] = static_cast<uint8_t>(value & 0xff);
        mem[(addr + 1) & 0xffff] = static_cast<uint8_t>(value >> 8);
    }

    void write32(uint16_t addr, uint32_t value) {
        write16(addr, static_cast<uint16_t>(value & 0xffff));
        write16(addr + 2, static_cast<uint16_t>(value >> 16));
    }

    std::string read_c_string(uint16_t addr) const {
        std::string s;
        for (int i = 0; i < 1024; ++i) {
            uint8_t ch = mem[(addr + i) & 0xffff];
            if (!ch)
                return s;
            s.push_back(static_cast<char>(ch));
        }
        return {};
    }

    std::string resolve_path(const std::string &guest_path) const {
        if (guest_path.empty() || guest_path[0] == '/' ||
            guest_path.find("..") != std::string::npos) {
            return {};
        }
        if (fs_root.empty() || fs_root == ".")
            return guest_path;
        if (fs_root.back() == '/')
            return fs_root + guest_path;
        return fs_root + "/" + guest_path;
    }

    int alloc_fd(std::unique_ptr<file_handle> fh) {
        for (size_t i = 0; i < files.size(); ++i) {
            if (!files[i]) {
                files[i] = std::move(fh);
                return static_cast<int>(i + 3);
            }
        }
        files.push_back(std::move(fh));
        return static_cast<int>(files.size() + 2);
    }

    file_handle *get_file(uint16_t fd) {
        if (fd < 3)
            return nullptr;
        size_t idx = static_cast<size_t>(fd - 3);
        if (idx >= files.size() || !files[idx])
            return nullptr;
        return files[idx].get();
    }

    void write_result16(int value) {
        write16(EMU_REQ_RESULT, static_cast<uint16_t>(value));
        write16(EMU_REQ_RESULT + 2, value < 0 ? 0xffff : 0);
    }

    void write_result32(int32_t value) {
        write32(EMU_REQ_RESULT, static_cast<uint32_t>(value));
    }

    void handle_open() {
        const std::string path = resolve_path(read_c_string(read16(EMU_REQ_PATH)));
        const uint16_t flags = read16(EMU_REQ_FLAGS);
        if (std::getenv("Z80_EXEC_TRACE")) {
            std::fprintf(stderr, "open path='%s' flags=0x%04x\n",
                         path.c_str(), flags);
        }
        if (path.empty()) {
            write_result16(-1);
            return;
        }

        std::ios::openmode mode = std::ios::binary;
        const uint16_t acc = flags & O_ACCMODE;
        if (acc == O_WRONLY)
            mode |= std::ios::out;
        else if (acc == O_RDWR)
            mode |= std::ios::in | std::ios::out;
        else
            mode |= std::ios::in;
        if (flags & O_TRUNC)
            mode |= std::ios::trunc;
        if (flags & O_APPEND)
            mode |= std::ios::app;

        if (flags & O_CREAT) {
            std::ofstream create(path, std::ios::binary | std::ios::app);
        }

        auto fh = std::make_unique<file_handle>();
        fh->flags = flags;
        fh->stream.open(path, mode);
        if (!fh->stream && (flags & O_CREAT) && acc == O_RDWR) {
            std::ofstream create(path, std::ios::binary);
            create.close();
            fh->stream.clear();
            fh->stream.open(path, mode);
        }
        if (!fh->stream) {
            write_result16(-1);
            return;
        }
        if (flags & O_APPEND) {
            fh->stream.seekg(0, std::ios::end);
            fh->stream.seekp(0, std::ios::end);
        }
        write_result16(alloc_fd(std::move(fh)));
    }

    void handle_close() {
        const uint16_t fd = read16(EMU_REQ_FD);
        if (fd < 3) {
            write_result16(0);
            return;
        }
        size_t idx = static_cast<size_t>(fd - 3);
        if (idx >= files.size() || !files[idx]) {
            write_result16(-1);
            return;
        }
        files[idx]->stream.close();
        files[idx].reset();
        write_result16(0);
    }

    void handle_read() {
        const uint16_t fd = read16(EMU_REQ_FD);
        const uint16_t ptr = read16(EMU_REQ_PTR);
        const uint16_t len = read16(EMU_REQ_LEN);
        if (fd == 0) {
            uint16_t n = 0;
            while (n < len && !console_in.empty()) {
                write8(ptr + n, console_in.front());
                console_in.pop_front();
                ++n;
            }
            write_result16(n);
            return;
        }
        auto *fh = get_file(fd);
        if (!fh) {
            write_result16(-1);
            return;
        }
        std::vector<char> buf(len);
        fh->stream.read(buf.data(), len);
        const auto n = static_cast<uint16_t>(fh->stream.gcount());
        for (uint16_t i = 0; i < n; ++i)
            write8(ptr + i, static_cast<uint8_t>(buf[i]));
        if (fh->stream.eof())
            fh->stream.clear();
        write_result16(n);
    }

    void handle_write() {
        const uint16_t fd = read16(EMU_REQ_FD);
        const uint16_t ptr = read16(EMU_REQ_PTR);
        const uint16_t len = read16(EMU_REQ_LEN);
        if (fd == 1 || fd == 2) {
            for (uint16_t i = 0; i < len; ++i)
                console_out.push_back(static_cast<char>(read8(ptr + i)));
            write_result16(len);
            return;
        }
        auto *fh = get_file(fd);
        if (!fh) {
            write_result16(-1);
            return;
        }
        for (uint16_t i = 0; i < len; ++i)
            fh->stream.put(static_cast<char>(read8(ptr + i)));
        fh->stream.flush();
        write_result16(fh->stream ? len : -1);
    }

    void handle_lseek() {
        const uint16_t fd = read16(EMU_REQ_FD);
        auto *fh = get_file(fd);
        if (!fh) {
            write_result32(-1);
            return;
        }
        const int32_t offset = static_cast<int32_t>(read32(EMU_REQ_OFFSET));
        const uint16_t whence = read16(EMU_REQ_WHENCE);
        if (std::getenv("Z80_EXEC_TRACE")) {
            std::fprintf(stderr, "lseek fd=%u offset=%ld whence=%u\n",
                         fd, static_cast<long>(offset), whence);
        }
        std::ios::seekdir dir = std::ios::beg;
        if (whence == 1)
            dir = std::ios::cur;
        else if (whence == 2)
            dir = std::ios::end;
        else if (whence != 0) {
            write_result32(-1);
            return;
        }
        const uint16_t acc = fh->flags & O_ACCMODE;
        const bool can_read = (acc == 0 || acc == O_RDWR);
        const bool can_write = (acc == O_WRONLY || acc == O_RDWR);
        fh->stream.clear();
        std::streamoff base = 0;
        if (dir == std::ios::beg) {
            base = 0;
        } else if (dir == std::ios::cur) {
            std::streampos pos = can_read ? fh->stream.tellg()
                                          : std::streampos(-1);
            if (pos < 0 && can_write)
                pos = fh->stream.tellp();
            if (pos < 0) {
                write_result32(-1);
                return;
            }
            base = static_cast<std::streamoff>(pos);
        } else {
            if (can_read) {
                fh->stream.seekg(0, std::ios::end);
                const std::streampos pos = fh->stream.tellg();
                if (pos >= 0)
                    base = static_cast<std::streamoff>(pos);
                else if (can_write)
                    base = static_cast<std::streamoff>(fh->stream.tellp());
                else
                    base = static_cast<std::streamoff>(-1);
            } else if (can_write) {
                fh->stream.seekp(0, std::ios::end);
                base = static_cast<std::streamoff>(fh->stream.tellp());
            } else {
                base = static_cast<std::streamoff>(-1);
            }
            if (base < 0) {
                write_result32(-1);
                return;
            }
        }
        const std::streamoff target = base + static_cast<std::streamoff>(offset);
        if (target < 0) {
            write_result32(-1);
            return;
        }
        if (can_read)
            fh->stream.seekg(target, std::ios::beg);
        if (can_write)
            fh->stream.seekp(target, std::ios::beg);
        std::streampos pos = can_read ? fh->stream.tellg()
                                      : std::streampos(-1);
        if (pos < 0 && can_write)
            pos = fh->stream.tellp();
        if (pos < 0) {
            write_result32(-1);
            return;
        }
        if (std::getenv("Z80_EXEC_TRACE")) {
            std::fprintf(stderr, "lseek result=%ld\n", static_cast<long>(pos));
        }
        write_result32(static_cast<int32_t>(pos));
    }

    void handle_unlink() {
        const std::string path = resolve_path(read_c_string(read16(EMU_REQ_PATH)));
        write_result16(!path.empty() && std::remove(path.c_str()) == 0 ? 0 : -1);
    }

    void handle_rename() {
        const std::string from = resolve_path(read_c_string(read16(EMU_REQ_PATH)));
        const std::string to = resolve_path(read_c_string(read16(EMU_REQ_PATH2)));
        write_result16(!from.empty() && !to.empty() &&
                       std::rename(from.c_str(), to.c_str()) == 0 ? 0 : -1);
    }

    void handle_command(uint8_t cmd) {
        switch (cmd) {
        case EMU_CMD_EXIT:
            done = true;
            write8(done_addr, done_magic);
            cpu.requestBreak();
            break;
        case EMU_CMD_OPEN:   handle_open(); break;
        case EMU_CMD_CLOSE:  handle_close(); break;
        case EMU_CMD_READ:   handle_read(); break;
        case EMU_CMD_WRITE:  handle_write(); break;
        case EMU_CMD_LSEEK:  handle_lseek(); break;
        case EMU_CMD_UNLINK: handle_unlink(); break;
        case EMU_CMD_RENAME: handle_rename(); break;
        default:
            break;
        }
    }

    void add_cycles(int clocks) {
        executed_cycles += clocks;
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
        } else if (arg == "--stdin") {
            opts.stdin_text = need_value("--stdin");
        } else if (arg == "--stdin-file") {
            opts.stdin_file = need_value("--stdin-file");
        } else if (arg == "--stdout") {
            opts.stdout_file = need_value("--stdout");
        } else if (arg == "--fs-root") {
            opts.fs_root = need_value("--fs-root");
        } else if (arg == "--print-output") {
            opts.print_output = true;
        } else if (arg == "--z88dk-trap") {
            opts.z88dk_trap = true;
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
                     "Usage: z80_exec [--ihx|--bin] [--cycles N] "
                     "[--fs-root DIR] [--stdin TEXT|--stdin-file FILE] "
                     "[--stdout FILE] [--print-output] "
                     "[--z88dk-trap] image\n");
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
    m.result_addr = opts.result_addr;
    m.fs_root = opts.fs_root;
    if (std::getenv("Z80_EXEC_TRACE_CPU")) {
        m.cpu.setDebugMessage([](void *, const char *msg) {
            std::fprintf(stderr, "%s\n", msg);
        });
    }
    m.cpu.setConsumeClockCallback([](void *arg, int clocks) {
        static_cast<machine *>(arg)->add_cycles(clocks);
    });

    for (unsigned char ch : opts.stdin_text)
        m.console_in.push_back(ch);
    if (!opts.stdin_file.empty()) {
        std::ifstream in(opts.stdin_file, std::ios::binary);
        if (!in)
            dief("cannot open stdin file '%s'", opts.stdin_file);
        std::vector<char> bytes((std::istreambuf_iterator<char>(in)),
                                std::istreambuf_iterator<char>());
        for (unsigned char ch : bytes)
            m.console_in.push_back(ch);
    }

    if (opts.format == "ihx")
        load_ihx(m, opts.image_path);
    else if (opts.format == "bin")
        load_binary(m, opts.image_path);
    else
        die("internal bad format");

    m.cpu.reg.PC = opts.start_pc;
    int executed = 0;
    bool z88dk_trapped = false;
    try {
        executed = m.cpu.execute(opts.cycle_budget);
    } catch (const std::exception &ex) {
        const uint16_t pc = m.cpu.reg.PC;
        if (opts.z88dk_trap &&
            m.mem[(pc - 2) & 0xffff] == 0xed &&
            m.mem[(pc - 1) & 0xffff] == 0xfe) {
            m.done = true;
            z88dk_trapped = true;
            executed = m.executed_cycles;
            m.write16(opts.result_addr,
                      static_cast<uint16_t>(m.cpu.reg.pair.L |
                      (static_cast<uint16_t>(m.cpu.reg.pair.H) << 8)));
        } else {
        std::fprintf(stderr,
                     "z80_exec: emulator exception at pc=0x%04x "
                     "bytes=%02x %02x %02x %02x: %s\n",
                     pc,
                     m.mem[pc],
                     m.mem[(pc + 1) & 0xffff],
                     m.mem[(pc + 2) & 0xffff],
                     m.mem[(pc + 3) & 0xffff],
                     ex.what());
        return 4;
        }
    }
    const uint16_t result = static_cast<uint16_t>(
        m.mem[opts.result_addr] |
        (static_cast<uint16_t>(m.mem[(opts.result_addr + 1) & 0xffff]) << 8));

    std::printf("done=%d return=%u cycles=%d pc=0x%04x\n",
                m.done ? 1 : 0, result, executed, m.cpu.reg.PC);
    if (z88dk_trapped && std::getenv("Z80_EXEC_TRACE")) {
        std::fprintf(stderr, "z88dk trap exit at pc=0x%04x\n", m.cpu.reg.PC);
    }

    if (!opts.stdout_file.empty()) {
        std::ofstream out(opts.stdout_file, std::ios::binary);
        if (!out)
            dief("cannot open stdout file '%s'", opts.stdout_file);
        out.write(m.console_out.data(),
                  static_cast<std::streamsize>(m.console_out.size()));
    }
    if (opts.print_output && !m.console_out.empty()) {
        std::fwrite(m.console_out.data(), 1, m.console_out.size(), stderr);
    }

    if (!m.done)
        return 2;
    return 0;
}
