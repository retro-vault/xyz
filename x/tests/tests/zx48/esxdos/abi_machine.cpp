// Independent esxDOS RST 8 contract model. This tests the target code and
// compiler's public ABI; it is not a substitute for the real-firmware run.
#include <xz80/xz80.h>
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <span>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using bytes = std::vector<uint8_t>;
static void require(bool good, const std::string &what) {
    if (!good) throw std::runtime_error(what);
}
static bytes read_file(const char *name) {
    std::ifstream in(name, std::ios::binary);
    require(bool(in), std::string("open ") + name);
    return bytes(std::istreambuf_iterator<char>(in), {});
}
struct disk_machine {
    xz80::flat_memory memory;
    xz80::null_ports ports;
    xz80::cpu cpu{memory, ports};
    std::map<std::string, bytes> files;
    std::set<std::string> directories{"/"};
    std::string cwd = "/";
    struct handle { std::string name; uint32_t pos{}; uint8_t mode{}; };
    std::map<uint8_t, handle> handles;
    std::map<std::string, uint16_t> symbols;
    std::map<unsigned, unsigned> calls;
    unsigned checks{}, services{};
    int fail_service = -1;
    int fail_handle = -1;
    uint8_t failure = 6;
    int64_t stat_size = -1;
    bool unterminated_cwd = false;
    bool rom_mode = false;
    bytes original_rom, expected_rom;
    int write_limit = -1, write_fail_after = -1;
    std::vector<unsigned> write_requests;
    unsigned last_stack_bytes{};
    uint64_t last_call_tstates{};

    disk_machine(std::span<const uint8_t> image, const char *map, bool rom = false) : rom_mode(rom) {
        if (rom_mode) {
            require(image.size() == 16384, "ROM image must be 16 KiB");
            original_rom.assign(image.begin(), image.end()); expected_rom = original_rom;
        }
        memory.load(rom_mode ? 0 : 0x8000, image);
        memory.write(0xff00, 0x76);
        std::ifstream in(map);
        std::string line;
        while (std::getline(in, line)) {
            std::istringstream text(line);
            std::string address, name;
            if (text >> address >> name && address.size() == 8) {
                try { symbols[name] = std::stoul(address, nullptr, 16); }
                catch (const std::exception &) {}
            }
        }
        if (rom_mode) {
            require(symbol("_entry") < 0x4000 && symbol("_main") < 0x4000
                    && symbol("__exit") < 0x4000, "ROM application was linked into RAM");
        }
        cpu.reset();
    }
    void check_rom() {
        if (!rom_mode) return;
        for (unsigned i = 0; i < expected_rom.size(); ++i)
            require(memory.read(i) == expected_rom[i], "target modified ROM byte " + std::to_string(i));
        unsigned gates = symbol("__zx_esx_gates_start");
        unsigned load = symbol("s__DATA_LOAD") + gates - symbol("s__DATA");
        require(symbol("__zx_esx_gate_b0") + 3u == gates + 48, "RAM gate extent");
        for (unsigned i = 0; i < 48; ++i)
            require(memory.read(gates + i) == expected_rom.at(load + i), "target modified RAM syscall gate");
    }
    void rom_fixture(uint16_t address, const bytes &data) {
        require(rom_mode && address >= 0x3d00 && unsigned(address) + data.size() <= 0x3e00,
                "ROM fixture must stay in reserved non-executable 3D00..3DFF hole");
        check_rom();
        for (unsigned i = 0; i < data.size(); ++i) {
            memory.write(address + i, data[i]); expected_rom[address + i] = data[i];
        }
    }
    void rom_string(uint16_t address, const std::string &value) {
        bytes data(value.begin(), value.end()); data.push_back(0); rom_fixture(address, data);
    }
    uint16_t symbol(const std::string &name) {
        require(symbols.contains(name), "missing symbol " + name);
        return symbols.at(name);
    }
    uint16_t word(uint16_t p) { return memory.read(p) | (memory.read(p + 1) << 8); }
    void word(uint16_t p, uint16_t value) {
        memory.write(p, value); memory.write(p + 1, value >> 8);
    }
    void dword(uint16_t p, uint32_t value) {
        word(p, value); word(p + 2, value >> 16);
    }
    std::string path(uint16_t p) {
        require(p >= 0x4000, "path in memory hidden by esxDOS ROM");
        std::string s;
        for (unsigned i = 0; i < 256; ++i) {
            uint8_t c = memory.read(p + i);
            if (!c) {
                std::string combined = !s.empty() && s[0] == '/' ? s : cwd + "/" + s;
                std::istringstream pieces(combined); std::string part;
                std::vector<std::string> components;
                while (std::getline(pieces, part, '/')) {
                    if (part.empty() || part == ".") continue;
                    if (part == "..") { if (!components.empty()) components.pop_back(); }
                    else components.push_back(part);
                }
                std::string result;
                for (const auto &component : components) result += "/" + component;
                return result.empty() ? "/" : result;
            }
            s.push_back(c);
        }
        throw std::runtime_error("unterminated firmware path");
    }
    void firmware() {
        auto s = cpu.snapshot();
        require(s.pc == 8, "firmware trap PC");
        uint16_t ret = word(s.sp);
        if (rom_mode) {
            auto gates = symbol("__zx_esx_gates_start");
            require(ret >= gates && ret < gates + 48 && (ret - gates) % 3 == 1,
                    "ROM program called firmware outside a fixed RAM gate");
        }
        unsigned service = memory.read(ret);
        ++calls[service]; ++services;
        auto input = s;
        // Every documented output is filled below; all other main registers,
        // IX and even IY are deliberately hostile, as is the alternate bank.
        s.af = 0x5500; s.bc = 0xa1a2; s.de = 0xb1b2; s.hl = 0xc1c2;
        s.ix = 0xd1d2; s.iy = 0xe1e2;
        s.af2 = 0x1234; s.bc2 = 0x5678; s.de2 = 0x9abc; s.hl2 = 0xdef0;
        s.sp += 2; s.pc = ret + 1;
        auto error = [&](uint8_t n) { s.af = uint16_t(n) << 8 | 1; };
        uint8_t id = input.af >> 8;
        if (int(service) == fail_service && (fail_handle < 0 || id == fail_handle)) { error(failure); cpu.restore(s); return; }
        if (service == 0x9e) {
            write_requests.push_back(input.bc);
            if (write_fail_after >= 0 && write_requests.size() > unsigned(write_fail_after)) {
                error(6); cpu.restore(s); return;
            }
        }
        if (service == 0x88) { s.ix = 0x0890; }
        else if (service == 0x89) { s.af = 0x0800; }
        else if (service == 0x9a) {
            require(id == '*', "F_OPEN current drive");
            std::string name = path(input.ix);
            uint8_t mode = input.bc >> 8;
            require((mode & 0xf0) == 0 && (mode & 3), "F_OPEN mode");
            bool exists = files.contains(name);
            if (!exists && !(mode & 0x0c)) error(5);
            else if (exists && (mode & 0x0c) == 4) error(18);
            else if (handles.size() == 16) error(12);
            else {
                if (!exists || (mode & 0x0c) == 12) files[name].clear();
                // Start with an inconvenient handle: the libc fd must still
                // be >= 3 and must not expose the firmware's handle space.
                uint8_t h = 7;
                while (handles.contains(h)) h = (h + 5) & 15;
                handles[h] = {name, 0, uint8_t(mode & 3)};
                s.af = uint16_t(h) << 8;
            }
        } else if (service == 0xad) {
            require(id == '*', "F_UNLINK current drive");
            auto name = path(input.ix);
            if (!files.erase(name)) error(5);
        } else if (service == 0xac) {
            require(id == '*', "F_STAT current drive");
            require(input.de >= 0x4000, "F_STAT output hidden by ROM");
            auto name = path(input.ix);
            if (!files.contains(name) && !directories.contains(name)) error(5);
            else {
                memory.write(input.de, 8); memory.write(input.de + 1, 0);
                memory.write(input.de + 2, directories.contains(name) ? 0x10 : 0x20);
                dword(input.de + 3, 0x12345678);
                dword(input.de + 7, stat_size >= 0 ? stat_size : files.contains(name) ? files.at(name).size() : UINT32_MAX);
            }
        } else if (service == 0xa8) {
            require(id == '*', "F_GETCWD current drive");
            if (unterminated_cwd) {
                for (unsigned i = 0; i < 256; ++i) memory.write(input.ix + i, 'x');
            } else for (unsigned i = 0; i <= cwd.size(); ++i)
                memory.write(input.ix + i, i == cwd.size() ? 0 : cwd[i]);
        } else if (service == 0xa9 || service == 0xaa || service == 0xab) {
            require(id == '*', "directory current drive");
            auto name = path(input.ix);
            if (service == 0xa9) {
                if (!directories.contains(name)) error(files.contains(name) ? 17 : 5);
                else cwd = name;
            } else if (service == 0xaa) {
                if (directories.contains(name) || files.contains(name)) error(18);
                else directories.insert(name);
            } else {
                bool nonempty = false;
                for (const auto &[file, data] : files) if (file.starts_with(name + "/")) nonempty = true;
                if (!directories.contains(name)) error(5);
                else if (nonempty || name == cwd) error(27);
                else directories.erase(name);
            }
        } else if (service == 0xb0) {
            require(id == '*', "F_RENAME current drive");
            auto name = path(input.ix), destination = path(input.de);
            if (!files.contains(name)) error(5);
            else if (files.contains(destination) || directories.contains(destination)) error(18);
            else { files[destination] = std::move(files[name]); files.erase(name); }
        } else if (service >= 0x9b && service <= 0xa2) {
            if (!handles.contains(id)) error(13);
            else {
                auto &h = handles.at(id);
                auto &file = files.at(h.name);
                switch (service) {
                case 0x9b: handles.erase(id); break;
                case 0x9c: break;
                case 0x9d: case 0x9e: {
                    require(input.ix >= 0x4000 || input.bc == 0,
                            "I/O buffer hidden by esxDOS ROM");
                    if (!(h.mode & (service == 0x9d ? 1 : 2))) { error(8); break; }
                    unsigned count = input.bc;
                    if (service == 0x9e && write_limit >= 0) count = std::min(count, unsigned(write_limit));
                    if (service == 0x9d) {
                        count = std::min<uint32_t>(count, h.pos < file.size() ? file.size() - h.pos : 0);
                        for (unsigned i = 0; i < count; ++i) memory.write(input.ix + i, file[h.pos + i]);
                    } else {
                        require(uint64_t(h.pos) + count <= 4 * 1024 * 1024, "unexpected gigantic modeled write");
                        if (h.pos + count > file.size()) file.resize(h.pos + count);
                        for (unsigned i = 0; i < count; ++i) file[h.pos + i] = memory.read(input.ix + i);
                    }
                    h.pos += count; s.bc = count; break;
                }
                case 0x9f: {
                    uint32_t amount = uint32_t(input.bc) << 16 | input.de;
                    uint8_t mode = input.ix & 255;
                    require(mode < 3, "F_SEEK whence");
                    if (mode == 2 && amount > h.pos) { error(15); break; }
                    uint64_t pos = mode == 0 ? amount : mode == 1 ? uint64_t(h.pos) + amount : h.pos - amount;
                    if (pos > UINT32_MAX) { error(15); break; }
                    h.pos = pos;
                    // F_SEEK has no documented output position. Poison BCDE;
                    // callers must obtain position explicitly with F_GETPOS.
                    break;
                }
                case 0xa0: s.bc = h.pos >> 16; s.de = h.pos; break;
                case 0xa1:
                    require(input.ix >= 0x4000, "F_FSTAT buffer hidden by ROM");
                    memory.write(input.ix, 8); memory.write(input.ix + 1, 0);
                    memory.write(input.ix + 2, 0x20);
                    dword(input.ix + 3, 0x12345678);
                    dword(input.ix + 7, stat_size >= 0 ? stat_size : file.size()); break;
                case 0xa2: {
                    uint32_t size = uint32_t(input.bc) << 16 | input.de;
                    if (!(h.mode & 2)) error(8);
                    else { require(size < 4 * 1024 * 1024, "huge F_TRUNCATE"); file.resize(size); }
                    break;
                }
                default: throw std::runtime_error("unexpected file service");
                }
            }
        } else throw std::runtime_error("unexpected RST 8 service " + std::to_string(service));
        cpu.restore(s);
    }
    xz80::cpu_state call(const std::string &name, uint16_t hl = 0, uint16_t de = 0,
                         const std::vector<uint16_t> &arguments = {}) {
        xz80::cpu_state state{};
        state.pc = symbol(name); state.sp = 0xfef0;
        for (auto i = arguments.rbegin(); i != arguments.rend(); ++i) {
            state.sp -= 2; word(state.sp, *i);
        }
        uint16_t expected_sp = state.sp;
        state.sp -= 2; word(state.sp, 0xff00);
        state.hl = hl; state.de = de; state.ix = 0x6a6b; state.iy = 0x5c3a;
        word(symbol("__errno_value"), 0);
        cpu.restore(state);
        const uint16_t entry_sp = state.sp;
        uint16_t minimum_sp = entry_sp;
        last_call_tstates = 0;
        for (unsigned steps = 0; steps < 3000000; ++steps) {
            if (cpu.snapshot().pc == 8) firmware();
            else last_call_tstates += cpu.step();
            auto result = cpu.snapshot();
            minimum_sp = std::min(minimum_sp, result.sp);
            if (result.pc == 0xff00) {
                last_stack_bytes = entry_sp - minimum_sp;
                require(result.sp == expected_sp, name + " corrupted caller stack");
                require(result.ix == 0x6a6b && result.iy == 0x5c3a, name + " corrupted IX/IY");
                check_rom();
                ++checks;
                return result;
            }
        }
        throw std::runtime_error(name + " call exceeded step budget");
    }
    void string(uint16_t address, const std::string &s) {
        for (unsigned i = 0; i <= s.size(); ++i) memory.write(address + i, i == s.size() ? 0 : s[i]);
    }
    unsigned error_number() { return word(symbol("__errno_value")); }
    uint32_t result32(const xz80::cpu_state &s) { return uint32_t(s.hl) << 16 | s.de; }
    void focused() {
        string(0x7000, "DIRECT.TMP");
        auto opened = call("_open", 0x7000, 0x0102);
        require(opened.de >= 3 && opened.de <= 18, "open fd range");
        uint16_t fd = opened.de;
        require(handles.size() == 1, "native handle count");
        auto &h = handles.begin()->second;
        std::vector<uint32_t> bases{0, 1, 65535, 65536, 0x123456, 0x7ffffffe, 0x7fffffff};
        std::vector<int32_t> offsets{INT32_MIN, -65537, -65536, -1, 0, 1, 65535, 65536, INT32_MAX};
        for (uint32_t base : bases) for (int32_t offset : offsets) {
            h.pos = base;
            unsigned before_seek = calls[0x9f];
            auto result = call("_lseek", fd, 0,
                {uint16_t(offset), uint16_t(uint32_t(offset) >> 16), 1});
            int64_t expected = int64_t(base) + offset;
            if (expected < 0 || expected > INT32_MAX) {
                require(result32(result) == UINT32_MAX, "out of range CUR accepted");
                require(error_number() == (expected < 0 ? 22u : 75u), "CUR range errno");
                require(calls[0x9f] == before_seek, "range error altered native position");
            } else require(result32(result) == uint32_t(expected), "32-bit CUR calculation");
        }
        h.pos = 123;
        for (int32_t offset : offsets) {
            auto result = call("_lseek", fd, 0,
                {uint16_t(offset), uint16_t(uint32_t(offset) >> 16), 0});
            require(result32(result) == (offset < 0 ? UINT32_MAX : uint32_t(offset)), "32-bit SET calculation");
        }
        for (uint32_t size : bases) for (int32_t offset : offsets) {
            stat_size = size;
            int64_t expected = int64_t(size) + offset;
            auto result = call("_lseek", fd, 0,
                {uint16_t(offset), uint16_t(uint32_t(offset) >> 16), 2});
            require(result32(result) == (expected < 0 || expected > INT32_MAX ? UINT32_MAX : uint32_t(expected)),
                    "32-bit END calculation");
        }
        stat_size = -1;
        for (uint16_t bad : {uint16_t(19), uint16_t(0x103), uint16_t(0xffff)}) {
            unsigned before = services;
            require(call("_close", bad).de == 0xffff && error_number() == 9, "bad fd close");
            require(call("_read", bad, 0x7100, {1}).de == 0xffff && error_number() == 9, "bad fd read");
            require(call("_write", bad, 0x7100, {1}).de == 0xffff && error_number() == 9, "bad fd write");
            require(result32(call("_lseek", bad, 0, {0, 0, 0})) == UINT32_MAX && error_number() == 9, "bad fd seek");
            require(services == before, "invalid fd reached firmware");
        }
        for (uint16_t console : {0, 1, 2}) {
            require(result32(call("_lseek", console, 0, {0, 0, 0})) == UINT32_MAX && error_number() == 29,
                    "console seek must be ESPIPE");
            require(call("_fstat", console, 0x7200).de == 0 && (word(0x7206) & 0xf000) == 0x2000,
                    "console fstat character device");
        }
        for (uint16_t bad : {uint16_t(0), uint16_t(0x3fff), uint16_t(0xfff3)}) {
            unsigned before = services;
            require(call("_fstat", fd, bad).de == 0xffff && error_number() == 14, "fstat bad buffer");
            require(call("_stat", 0x7000, bad).de == 0xffff && error_number() == 14, "stat bad buffer");
            require(services == before, "bad stat span reached firmware");
        }
        for (unsigned i = 0; i < 14; ++i) memory.write(0x7200 + i, 0x55);
        stat_size = 0x80000000LL;
        require(call("_fstat", fd, 0x7200).de == 0xffff && error_number() == 75, "fstat signed size overflow");
        for (unsigned i = 0; i < 14; ++i) require(memory.read(0x7200 + i) == 0x55, "fstat overflow touched buffer");
        stat_size = -1;
        for (uint16_t bad : rom_mode ? std::vector<uint16_t>{0} : std::vector<uint16_t>{0,0x3fff}) {
            unsigned before = services;
            require(call("_open", bad, 0).de == 0xffff && error_number() == 14, "open bad path");
            require(call("_rename", 0x7000, bad).de == 0xffff && error_number() == 14, "rename bad second path");
            require(services == before, "bad path reached firmware");
        }
        string(0x7300, std::string(256, 'X'));
        require(call("_unlink", 0x7300).de == 0xffff && error_number() == 36, "long path errno");
        string(0x7300, "");
        require(call("_unlink", 0x7300).de == 0xffff && error_number() == 2, "empty path errno");
        require(call("_getcwd", 0x7400, 2).de == 0x7400 && word(0x7400) == '/', "getcwd exact fit");
        memory.write(0x7400, 0x55);
        require(call("_getcwd", 0x7400, 1).de == 0 && error_number() == 34 && memory.read(0x7400) == 0x55,
                "small getcwd buffer untouched");
        unterminated_cwd = true;
        require(call("_getcwd", 0x7400, 256).de == 0 && error_number() == 34 && memory.read(0x7400) == 0x55,
                "unterminated native getcwd rejected");
        unterminated_cwd = false;
        cwd = "/" + std::string(254, 'x');
        require(call("_getcwd", 0x7400, 256).de == 0x7400 && memory.read(0x74ff) == 0,
                "getcwd maximum exact fit");
        cwd = "/";
        require(call("_getcwd", 0, 1).de == 0 && error_number() == 22, "getcwd NULL contract");
        require(call("_getcwd", 0x7400, 0).de == 0 && error_number() == 22, "getcwd zero size");
        require(call("_getcwd", 0xffff, 2).de == 0 && error_number() == 14, "getcwd wrapping span");
        for (const std::string op : {"_read", "_write"}) {
            for (auto [buffer, count] : std::vector<std::pair<uint16_t, uint16_t>>{{0,1},{0x3fff,1},{0xffff,2}}) {
                if (rom_mode && op == "_write" && buffer == 0x3fff) continue;
                unsigned before = services;
                require(call(op, fd, buffer, {count}).de == 0xffff && error_number() == 14, op + " invalid span");
                require(services == before, op + " invalid span reached firmware");
            }
            require(call(op, fd, 0, {0}).de == 0, op + " zero length ignores NULL");
        }
        require(call("_fstat", fd, 0xfff2).de == 0, "fstat last legal 14-byte span");
        for (uint16_t flags : {uint16_t(3), uint16_t(0x1000), uint16_t(0x0800), uint16_t(0x0200)}) {
            unsigned before = services;
            require(call("_open", 0x7000, flags).de == 0xffff && error_number() == 22, "invalid open flags");
            require(services == before, "invalid flags reached firmware");
        }
        // Exhaust every documented native error, including unknown values.
        // Target errno numbers are fixed by x/libc/include/errno.h and
        // intentionally independent of the host operating system's errno.
        const unsigned expected_errors[32] = {
            5,5,22,22,22,2,5,22,       // invalid requests, missing file, I/O
            13,28,6,19,24,9,19,75,    // access, space, device, handles, overflow
            21,20,17,2,38,36,2,16,    // directory, exists, path, unsupported, busy
            30,5,5,39,16,16,19,16     // read-only, nonempty, busy, filesystem
        };
        fail_service = 0x9c;
        for (unsigned code = 0; code < 256; ++code) {
            failure = code;
            require(call("_fsync", fd).de == 0xffff, "native failure return");
            require(error_number() == (code < 32 ? expected_errors[code] : 5), "native errno translation");
        }
        fail_service = 0x9b; failure = 6;
        require(call("_close", fd).de == 0xffff && error_number() == 5, "close native failure");
        fail_service = -1;
        require(call("_fsync", fd).de == 0, "failed close lost live descriptor");
        require(call("_close", fd).de == 0, "close after failure");
        require(call("_unlink", 0x7000).de == 0, "direct test cleanup");
        std::vector<uint16_t> descriptors;
        for (unsigned i = 0; i < 16; ++i) {
            string(0x7000, "SLOT" + std::to_string(i) + ".TMP");
            auto result = call("_open", 0x7000, 0x0102);
            require(result.de >= 3 && result.de <= 18, "descriptor capacity");
            descriptors.push_back(result.de);
        }
        unsigned before = services;
        string(0x7000, "SLOT16.TMP");
        require(call("_open", 0x7000, 0x0102).de == 0xffff && error_number() == 24,
                "descriptor table full errno");
        require(services == before, "full descriptor table leaked firmware handle");
        for (unsigned i = 0; i < descriptors.size(); ++i) {
            require(call("_close", descriptors[i]).de == 0, "close full descriptor table");
            string(0x7000, "SLOT" + std::to_string(i) + ".TMP");
            require(call("_unlink", 0x7000).de == 0, "slot test cleanup");
        }
    }
    void rom_focused() {
        require(rom_mode, "ROM-only focused tests");
        bool stack_bounds_ok = true;
        auto path_stack = [&](const std::string &label, unsigned limit) {
            // Includes the RAM gate and RST return words; excludes the
            // public caller's arguments/return word and modeled firmware.
            std::cout << "STACK " << label << " bytes=" << last_stack_bytes
                      << " limit=" << limit
                      << " wrapper_tstates=" << last_call_tstates << '\n';
            stack_bounds_ok &= last_stack_bytes <= limit;
        };
        // Fixture bytes occupy only a linker-reserved hole. Application
        // code and constants are never patched; every target call checks
        // the complete ROM against the expected read-only image.
        rom_string(0x3d00, std::string(255, 'P'));
        auto fd = call("_open", 0x3d00, 0x0102).de;
        require(fd >= 3 && fd <= 18, "255-byte ROM pathname rejected");
        path_stack("open-255", 32 + 256);
        require(call("_close", fd).de == 0, "close long ROM pathname");
        require(call("_rename", 0x3d00, 0x3d00).de == 0xffff
                    && error_number() == 17,
                "same maximum ROM paths must preserve native EEXIST");
        path_stack("rename-255-255", 22 + 512);
        require(call("_rename", 0x3d00, 0x3d01).de == 0,
                "overlapping 255/254-byte ROM paths");
        path_stack("rename-255-254", 22 + 511);
        require(call("_unlink", 0x3d01).de == 0,
                "unlink renamed long ROM pathname");
        for (unsigned length : {1u, 2u, 127u, 128u, 254u}) {
            rom_string(0x3d00, std::string(length, 'P'));
            fd = call("_open", 0x3d00, 0x0102).de;
            require(fd >= 3 && fd <= 18, "bounded ROM pathname open");
            path_stack("open-" + std::to_string(length), 33 + length);
            require(call("_close", fd).de == 0, "close bounded ROM path");
            require(call("_unlink", 0x3d00).de == 0,
                    "unlink bounded ROM pathname");
            path_stack("unlink-" + std::to_string(length), 23 + length);
        }
        rom_fixture(0x3d00, bytes(256, 'P'));
        unsigned before = services;
        require(call("_open", 0x3d00, 0).de == 0xffff && error_number() == 36,
                "256-byte ROM pathname must be ENAMETOOLONG");
        require(services == before, "overlong ROM path reached firmware");
        rom_string(0x3d00, "");
        require(call("_open", 0x3d00, 0).de == 0xffff && error_number() == 2,
                "empty ROM pathname must be ENOENT");

        rom_string(0x3d00, "ROM-A.TMP"); rom_string(0x3d80, "ROM-B.TMP");
        fd = call("_open", 0x3d00, 0x0102).de;
        require(fd >= 3 && fd <= 18, "ROM path open");
        path_stack("open-short", 32 + 10);
        require(call("_close", fd).de == 0, "ROM path close");
        require(call("_stat", 0x3d00, 0x7200).de == 0, "ROM path stat");
        path_stack("stat-short", 40 + 10);
        fd = call("_open", 0x3d00, 0x0202).de;
        require(fd >= 3 && fd <= 18, "ROM path truncation without create");
        path_stack("open-truncate-short", 40 + 10);
        require(call("_close", fd).de == 0, "close truncated ROM path");
        string(0x7000, "RAM-C.TMP"); string(0x7100, "RAM-D.TMP");
        require(call("_rename", 0x3d00, 0x3d80).de == 0, "rename ROM to ROM");
        path_stack("rename-short-ROM-ROM", 22 + 20);
        require(call("_rename", 0x3d80, 0x7000).de == 0, "rename ROM to RAM");
        path_stack("rename-short-ROM-RAM", 22 + 10);
        require(call("_rename", 0x7000, 0x3d00).de == 0, "rename RAM to ROM");
        path_stack("rename-short-RAM-ROM", 22 + 10);
        require(call("_rename", 0x3d00, 0x7000).de == 0, "rename ROM to RAM again");
        require(call("_rename", 0x7000, 0x7100).de == 0, "rename RAM to RAM");
        path_stack("rename-short-RAM-RAM", 22);
        require(call("_unlink", 0x7100).de == 0, "mixed rename cleanup");
        require(call("_mkdir", 0x3d00, 0777).de == 0, "mkdir ROM path");
        require(call("_chdir", 0x3d00).de == 0, "chdir ROM path");
        string(0x7000, ".."); require(call("_chdir", 0x7000).de == 0, "chdir parent");
        require(call("_rmdir", 0x3d00).de == 0, "rmdir ROM path");

        const uint16_t source = 0x0800;
        const unsigned length = 513;
        bytes expected;
        for (unsigned i = 0; i < length; ++i) expected.push_back(memory.read(source + i));
        auto fresh_file = [&]() {
            auto result = call("_open", 0x3d00, 0x0302).de;
            require(result >= 3 && result <= 18, "open ROM write fixture");
            write_requests.clear(); return result;
        };
        fd = fresh_file();
        require(call("_write", fd, source, {length}).de == length, "multi-chunk ROM write count");
        require(write_requests.size() > 1, "ROM write did not exercise multiple firmware calls");
        require(files.at("/ROM-A.TMP") == expected, "multi-chunk ROM write bytes");
        require(call("_close", fd).de == 0, "close multi-chunk write");
        fd = fresh_file(); write_limit = 7;
        require(call("_write", fd, source, {length}).de == 7, "short ROM write return count");
        require(write_requests.size() == 1, "short native write was retried");
        require(files.at("/ROM-A.TMP") == bytes(expected.begin(), expected.begin() + 7), "short ROM write bytes");
        write_limit = -1; require(call("_close", fd).de == 0, "close short write");
        fd = fresh_file(); write_limit = 0;
        require(call("_write", fd, source, {length}).de == 0, "zero native write must terminate");
        require(write_requests.size() == 1 && files.at("/ROM-A.TMP").empty(), "zero native write looped or altered file");
        write_limit = -1; require(call("_close", fd).de == 0, "close zero write");
        fd = fresh_file(); write_fail_after = 1;
        require(call("_write", fd, source, {length}).de == 0xffff && error_number() == 5,
                "error after a successful chunk must preserve backend -1/EIO contract");
        require(write_requests.size() == 2, "write continued after native error");
        require(files.at("/ROM-A.TMP") == bytes(expected.begin(), expected.begin() + write_requests.front()),
                "error after a chunk lost or duplicated confirmed file prefix");
        write_fail_after = -1; require(call("_close", fd).de == 0, "close failed write");
        fd = fresh_file();
        require(call("_write", fd, 0x3fff, {1}).de == 1, "last ROM byte is readable for write");
        require(files.at("/ROM-A.TMP") == bytes{memory.read(0x3fff)}, "last ROM byte write contents");
        require(call("_close", fd).de == 0 && call("_unlink", 0x3d00).de == 0, "ROM write cleanup");
        for (unsigned i = 0x3d00; i < 0x3e00; ++i) memory.write(i, original_rom[i]);
        expected_rom = original_rom; check_rom();
        require(stack_bounds_ok, "ROM path scratch exceeds pathname lengths");
    }
    void exit_test(bool inject_failure) {
        std::vector<uint16_t> fds;
        string(0x7100, "preserved");
        for (unsigned i = 0; i < 3; ++i) {
            string(0x7000, "EXIT" + std::to_string(i) + ".TMP");
            auto fd = call("_open", 0x7000, 0x0102).de;
            require(fd >= 3 && fd <= 18, "exit test open");
            require(call("_write", fd, 0x7100, {9}).de == 9, "exit test write");
            fds.push_back(fd);
        }
        if (inject_failure) { fail_service = 0x9b; fail_handle = handles.begin()->first; failure = 6; }
        unsigned before = calls[0x9b];
        xz80::cpu_state state{};
        state.pc = symbol("__exit"); state.sp = 0xfef0; state.hl = 0x1234;
        state.ix = 0x6a6b; state.iy = 0x5c3a;
        cpu.restore(state);
        for (unsigned steps = 0; steps < 100000; ++steps) {
            if (cpu.snapshot().pc == 8) firmware(); else cpu.step();
            if (cpu.halted()) break;
        }
        require(cpu.halted(), "exit did not halt within finite budget");
        require(word(symbol("_zx_exit_status")) == 0x1234, "exit lost status across firmware calls");
        require(calls[0x9b] - before == 3, "exit did not attempt every active descriptor");
        require(handles.size() == (inject_failure ? 1u : 0u), "exit handle close result");
        require(cpu.snapshot().sp == 0xfef0, "exit leaked stack");
        check_rom();
        ++checks;
        fail_service = -1; fail_handle = -1;
        for (unsigned i = 0; i < fds.size(); ++i) {
            call("_close", fds[i]);
            auto name = "/EXIT" + std::to_string(i) + ".TMP";
            require(files.at(name) == bytes({'p','r','e','s','e','r','v','e','d'}), "exit damaged pending file contents");
            string(0x7000, name);
            require(call("_unlink", 0x7000).de == 0, "exit test cleanup");
        }
    }
    void run(uint16_t entry, uint16_t marker) {
        xz80::cpu_state state{};
        state.pc = entry; state.sp = 0xfef0; state.ix = 0x4b4c; state.iy = 0x5c3a;
        cpu.restore(state);
        for (unsigned long steps = 0; steps < 30000000; ++steps) {
            if (cpu.snapshot().pc == 8) firmware(); else cpu.step();
            unsigned result = word(marker);
            if (result == 0xa55a) { check_rom(); ++checks; return; }
            if (result) throw std::runtime_error("C disk probe failed phase " + std::to_string(result));
            if (cpu.halted()) throw std::runtime_error("C disk probe halted without success");
        }
        throw std::runtime_error("C disk probe step budget exhausted");
    }
};
int main(int argc, char **argv) {
    try {
        require(argc == 3 || (argc == 4 && std::string(argv[3]) == "--rom"),
                "usage: abi_machine image.bin image.map [--rom]");
        disk_machine m(read_file(argv[1]), argv[2], argc == 4);
        m.run(m.symbol(m.rom_mode ? "__zx_rom_startup" : "_entry"), m.symbol("_zx_disk_result"));
        m.focused();
        if (m.rom_mode) m.rom_focused();
        m.exit_test(false);
        m.exit_test(true);
        require(m.handles.empty(), "leaked firmware handles");
        require(m.files.empty(), "probe did not remove its files");
        for (unsigned id : {0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 0xa0, 0xa1, 0xac, 0xad})
            require(m.calls[id] != 0, "untested firmware service " + std::to_string(id));
        std::cout << "PASS esxDOS public C ABI with hostile firmware registers ("
                  << m.checks << " direct/compiled checks, " << m.services << " firmware calls)\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n'; return 1;
    }
}
