#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <microdrive/microdrive.h>

extern "C" {
#include "vendor/superzazu-z80/z80.h"
}

namespace fs = std::filesystem;

namespace {

constexpr uint16_t kTrapAddr = 0xFF00;
constexpr uint16_t kStackTop = 0xFFFC;
constexpr uint64_t kMaxSteps = 8'000'000;
constexpr uint64_t kMaxStepsStrict = 200'000'000;
constexpr uint16_t kRomSize = 16 * 1024;

constexpr uint8_t kPortMdData = 0xE7;
constexpr uint8_t kPortMdCtrl = 0xEF;
constexpr uint8_t kPortMdSel = 0xF7;

constexpr uint8_t kStatusIdle = 0xF7;
constexpr uint8_t kStatusGapSyncLow = 0xF9;
constexpr uint8_t kSyncOk = 0xFF;
constexpr uint8_t kSyncNo = 0x00;

constexpr uint16_t kNameAddr = 0xC000;
constexpr uint16_t kDirAddr = 0xC100;
constexpr uint16_t kLoadAddr = 0xC400;
constexpr uint16_t kSaveAddr = 0xD000;
constexpr std::size_t kDirEntrySize = 14;
const fs::path kYosRomRelativePath = "bin/y/z80/spectrum/bin/yos.rom";
const fs::path kYosCdbRelativePath = "bin/y/z80/spectrum/bin/yos.cdb";

bool g_strict_timing = false;
bool g_trace_writes = false;

struct WriteEvent {
    int drive = 0;
    int pos = 0;
    uint8_t ctrl = 0;
    uint8_t value = 0;
};

std::vector<WriteEvent> g_write_events;

struct DriveState {
    bool inserted = false;
    bool motor_on = false;
    bool modified = false;
    std::optional<microdrive::image_t> image;
    std::vector<uint8_t> preamble;
    int head_pos = 0;
    int transferred = 0;
    int max_bytes = 15;
    int gap = 15;
    int sync = 15;
    uint8_t last = 0xFF;
};

struct PortState {
    std::array<DriveState, 8> drives {};
    uint8_t last_ctrl_out = 0xFF;
    uint8_t last_sel_out = 0x00;
    bool ctrl_clock_high = true;
    uint64_t in_count = 0;
    uint64_t out_count = 0;
};

struct Machine {
    std::array<uint8_t, 65536> mem {};
    z80 cpu {};
    PortState ports {};
};

enum class If1Port {
    Mdr,
    Ctr,
    Net,
    Unknown,
};

struct CallSetup {
    std::optional<uint8_t> a;
    std::optional<uint8_t> b;
    std::optional<uint8_t> c;
    std::optional<uint8_t> d;
    std::optional<uint8_t> e;
    std::optional<uint8_t> h;
    std::optional<uint8_t> l;
    std::vector<uint8_t> stack_args;
};

struct CallResult {
    uint8_t a = 0;
    uint8_t l = 0;
    uint16_t hl = 0;
    uint64_t steps = 0;
};

std::unordered_map<std::string, uint16_t> load_symbols(const fs::path& debug_path) {
    std::ifstream in(debug_path);
    if (!in) {
        throw std::runtime_error("cannot open debug file: " + debug_path.string());
    }

    std::unordered_map<std::string, uint16_t> symbols;
    std::string line;

    if (debug_path.extension() == ".noi") {
        while (std::getline(in, line)) {
            std::istringstream iss(line);
            std::string def;
            std::string name;
            std::string value;
            if (!(iss >> def >> name >> value))
                continue;
            if (def == "DEF" && value.starts_with("0x")) {
                symbols[name] = static_cast<uint16_t>(
                    std::stoul(value, nullptr, 16));
            }
        }
        return symbols;
    }

    if (debug_path.extension() == ".cdb") {
        std::string current_module;
        while (std::getline(in, line)) {
            if (line.rfind("M:", 0) == 0) {
                current_module = line.substr(2);
                continue;
            }

            if (current_module.empty() || line.rfind("L:A$", 0) != 0) {
                continue;
            }

            const auto colon = line.rfind(':');
            if (colon == std::string::npos || colon + 1 >= line.size()) {
                continue;
            }

            symbols["_" + current_module] = static_cast<uint16_t>(
                std::stoul(line.substr(colon + 1), nullptr, 16));
            current_module.clear();
        }
        return symbols;
    }

    throw std::runtime_error("unsupported debug file format: " + debug_path.string());
}

void load_rom(Machine& machine, const fs::path& rom_path) {
    std::ifstream in(rom_path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("cannot open ROM: " + rom_path.string());
    }
    in.read(reinterpret_cast<char*>(machine.mem.data()), kRomSize);
    if (in.gcount() != kRomSize) {
        throw std::runtime_error("unexpected ROM size in " + rom_path.string());
    }
}

uint8_t read_byte(void* userdata, uint16_t addr) {
    auto* machine = static_cast<Machine*>(userdata);
    return machine->mem[addr];
}

void write_byte(void* userdata, uint16_t addr, uint8_t value) {
    auto* machine = static_cast<Machine*>(userdata);
    if (addr < kRomSize) {
        return;
    }
    machine->mem[addr] = value;
}

void increment_head(DriveState& drive) {
    if (!drive.image.has_value()) {
        return;
    }

    drive.head_pos++;
    const int limit = drive.image->block_count() * microdrive::k_sector_size;
    if (drive.head_pos >= limit) {
        drive.head_pos = 0;
    }
}

void restart_drive(DriveState& drive) {
    while ((drive.head_pos % microdrive::k_sector_size) != 0 &&
           (drive.head_pos % microdrive::k_sector_size) != microdrive::k_off_record) {
        increment_head(drive);
    }

    drive.transferred = 0;
    drive.max_bytes = (drive.head_pos % microdrive::k_sector_size) == 0
                          ? 15
                          : 15 + microdrive::k_data_size + 1;
}

void restart_drives(PortState& ports) {
    for (auto& drive : ports.drives) {
        if (drive.inserted && drive.image.has_value()) {
            restart_drive(drive);
        }
    }
}

void attach_drive(Machine& machine, int drive_index, microdrive::image_t image) {
    auto& drive = machine.ports.drives.at(static_cast<std::size_t>(drive_index));
    drive.inserted = true;
    drive.motor_on = false;
    drive.modified = false;
    drive.head_pos = 0;
    drive.transferred = 0;
    drive.max_bytes = 15;
    drive.gap = 15;
    drive.sync = 15;
    drive.last = 0xFF;
    drive.preamble.assign(static_cast<std::size_t>(image.block_count()) * 2, kSyncOk);
    drive.image = std::move(image);
}

void attach_blank_drive(Machine& machine, int drive_index, std::string_view cart_name) {
    attach_drive(machine, drive_index, microdrive::image_t::create_blank(cart_name));
}

void prepare_stream_byte(DriveState& drive) {
    if (!drive.image.has_value()) {
        return;
    }

    if (drive.transferred >= drive.max_bytes) {
        restart_drive(drive);
    }
}

If1Port decode_if1_port(uint8_t port) {
    switch (port & 0x18) {
        case 0x00: return If1Port::Mdr;
        case 0x08: return If1Port::Ctr;
        case 0x10: return If1Port::Net;
        default: return If1Port::Unknown;
    }
}

uint8_t port_in(z80* cpu, uint8_t port) {
    auto* machine = static_cast<Machine*>(cpu->userdata);
    machine->ports.in_count++;

    switch (decode_if1_port(port)) {
        case If1Port::Ctr: {
            uint8_t ret = kStatusIdle;
            for (auto& drive : machine->ports.drives) {
                if (!drive.motor_on || !drive.inserted || !drive.image.has_value()) {
                    continue;
                }

                const int block = drive.head_pos / microdrive::k_sector_size +
                                  (drive.max_bytes == 15 ? 0 : drive.image->block_count());

                if (drive.preamble[block] == kSyncOk) {
                    if (drive.gap > 0) {
                        drive.gap--;
                    } else {
                        ret &= kStatusGapSyncLow;
                        if (drive.sync > 0) {
                            drive.sync--;
                        } else {
                            drive.gap = 15;
                            drive.sync = 15;
                        }
                    }
                }

                if (drive.image->write_protected()) {
                    ret &= 0xFE;
                }

            }
            restart_drives(machine->ports);
            return ret;
        }

        case If1Port::Mdr: {
            uint8_t ret = 0xFF;
            for (auto& drive : machine->ports.drives) {
                if (!drive.motor_on || !drive.inserted || !drive.image.has_value()) {
                    continue;
                }

                if (g_strict_timing) {
                    if (drive.transferred < drive.max_bytes) {
                        drive.last = drive.image->raw_byte(static_cast<std::size_t>(drive.head_pos));
                        increment_head(drive);
                    }
                } else {
                    prepare_stream_byte(drive);
                    drive.last = drive.image->raw_byte(static_cast<std::size_t>(drive.head_pos));
                    increment_head(drive);
                }
                drive.transferred++;
                ret &= drive.last;
            }
            return ret;
        }

        case If1Port::Net:
            if (g_strict_timing) {
                restart_drives(machine->ports);
            }
            return 0xFF;

        default:
            return 0xFF;
    }
}

void port_out(z80* cpu, uint8_t port, uint8_t value) {
    auto* machine = static_cast<Machine*>(cpu->userdata);
    machine->ports.out_count++;

    switch (decode_if1_port(port)) {
        case If1Port::Mdr:
            for (auto& drive : machine->ports.drives) {
                if (!drive.motor_on || !drive.inserted || !drive.image.has_value() ||
                    drive.image->write_protected()) {
                    continue;
                }

                bool can_write = true;
                if (g_strict_timing) {
                    const bool write_mode = (machine->ports.last_ctrl_out & 0x04) == 0;
                    const bool erase_active = (machine->ports.last_ctrl_out & 0x08) != 0;
                    can_write = write_mode && erase_active;
                }

                if (!g_strict_timing && drive.transferred >= drive.max_bytes + 12) {
                    restart_drive(drive);
                }

                const int block =
                    drive.head_pos / microdrive::k_sector_size +
                    (drive.max_bytes == 15 ? 0 : drive.image->block_count());
                if (drive.transferred == 0 && value == 0x00) {
                    drive.preamble[static_cast<std::size_t>(block)] = 1;
                } else if (drive.transferred > 0 && drive.transferred < 10 && value == 0x00) {
                    drive.preamble[static_cast<std::size_t>(block)]++;
                } else if (drive.transferred > 9 && drive.transferred < 12 && value == 0xFF) {
                    drive.preamble[static_cast<std::size_t>(block)]++;
                } else if (drive.transferred == 12 &&
                           drive.preamble[static_cast<std::size_t>(block)] == 12) {
                    drive.preamble[static_cast<std::size_t>(block)] = kSyncOk;
                }

                if (drive.transferred > 11 && drive.transferred < drive.max_bytes + 12 &&
                    can_write) {
                    if (g_trace_writes && g_write_events.size() < 128) {
                        g_write_events.push_back({
                            .drive = static_cast<int>(&drive - machine->ports.drives.data()),
                            .pos = drive.head_pos,
                            .ctrl = machine->ports.last_ctrl_out,
                            .value = value,
                        });
                    }
                    drive.image->set_raw_byte(static_cast<std::size_t>(drive.head_pos), value);
                    increment_head(drive);
                    drive.modified = true;
                }
                drive.transferred++;
            }
            break;

        case If1Port::Ctr: {
            const bool new_clock_high = (value & 0x02) != 0;
            if (!new_clock_high && machine->ports.ctrl_clock_high) {
                for (std::size_t i = machine->ports.drives.size() - 1; i > 0; --i) {
                    machine->ports.drives[i].motor_on = machine->ports.drives[i - 1].motor_on;
                }
                if (g_strict_timing) {
                    machine->ports.drives[0].motor_on = (value & 0x01) == 0;
                } else {
                    machine->ports.drives[0].motor_on = (machine->ports.last_sel_out & 0x01) != 0;
                }
            }

            machine->ports.ctrl_clock_high = new_clock_high;
            machine->ports.last_ctrl_out = value;
            if (g_strict_timing) {
                restart_drives(machine->ports);
            }
            break;
        }

        case If1Port::Net:
            machine->ports.last_sel_out = value;
            if (g_strict_timing) {
                restart_drives(machine->ports);
            }
            break;

        default:
            break;
    }
}

void init_cpu(Machine& machine) {
    z80_init(&machine.cpu);
    machine.cpu.userdata = &machine;
    machine.cpu.read_byte = read_byte;
    machine.cpu.write_byte = write_byte;
    machine.cpu.port_in = port_in;
    machine.cpu.port_out = port_out;
}

void poke_bytes(Machine& machine, uint16_t addr, std::span<const uint8_t> bytes) {
    std::copy(bytes.begin(), bytes.end(), machine.mem.begin() + addr);
}

CallResult call_function(Machine& machine, uint16_t addr, const CallSetup& setup = {}) {
    init_cpu(machine);

    const uint16_t sp = static_cast<uint16_t>(kStackTop - setup.stack_args.size());
    if (sp < kRomSize) {
        throw std::runtime_error("stack setup overlaps ROM");
    }

    machine.mem[kTrapAddr] = 0x76;
    machine.mem[sp] = static_cast<uint8_t>(kTrapAddr & 0xFF);
    machine.mem[sp + 1] = static_cast<uint8_t>(kTrapAddr >> 8);
    if (!setup.stack_args.empty()) {
        std::copy(setup.stack_args.begin(), setup.stack_args.end(), machine.mem.begin() + sp + 2);
    }

    machine.cpu.pc = addr;
    machine.cpu.sp = sp;

    if (setup.a) machine.cpu.a = *setup.a;
    if (setup.b) machine.cpu.b = *setup.b;
    if (setup.c) machine.cpu.c = *setup.c;
    if (setup.d) machine.cpu.d = *setup.d;
    if (setup.e) machine.cpu.e = *setup.e;
    if (setup.h) machine.cpu.h = *setup.h;
    if (setup.l) machine.cpu.l = *setup.l;

    uint64_t steps = 0;
    const uint64_t step_limit = g_strict_timing ? kMaxStepsStrict : kMaxSteps;
    while (!machine.cpu.halted && steps < step_limit) {
        z80_step(&machine.cpu);
        steps++;
    }

    if (!machine.cpu.halted) {
        std::ostringstream oss;
        oss << "CPU did not halt before step limit"
            << " pc=0x" << std::hex << std::uppercase << machine.cpu.pc
            << " sp=0x" << machine.cpu.sp
            << " a=0x" << unsigned(machine.cpu.a)
            << " b=0x" << unsigned(machine.cpu.b)
            << " c=0x" << unsigned(machine.cpu.c)
            << " d=0x" << unsigned(machine.cpu.d)
            << " e=0x" << unsigned(machine.cpu.e)
            << " h=0x" << unsigned(machine.cpu.h)
            << " l=0x" << unsigned(machine.cpu.l)
            << std::dec
            << " steps=" << steps
            << " in=" << machine.ports.in_count
            << " out=" << machine.ports.out_count;
        throw std::runtime_error(oss.str());
    }

    return {
        .a = machine.cpu.a,
        .l = machine.cpu.l,
        .hl = static_cast<uint16_t>((machine.cpu.h << 8) | machine.cpu.l),
        .steps = steps,
    };
}

std::string read_c_string(const Machine& machine, uint16_t addr, std::size_t max_len) {
    std::string out;
    for (std::size_t i = 0; i < max_len; ++i) {
        const char ch = static_cast<char>(machine.mem[addr + i]);
        if (ch == '\0') {
            break;
        }
        out.push_back(ch);
    }
    return out;
}

std::string trim_right_spaces(std::string s) {
    while (!s.empty() && s.back() == ' ') {
        s.pop_back();
    }
    return s;
}

std::vector<uint8_t> make_pattern(std::size_t size) {
    std::vector<uint8_t> out(size);
    for (std::size_t i = 0; i < size; ++i) {
        out[i] = static_cast<uint8_t>((i % 11) + 1);
    }
    return out;
}

void write_name(Machine& machine, uint16_t addr, std::string_view name) {
    std::vector<uint8_t> bytes(name.begin(), name.end());
    bytes.push_back(0);
    poke_bytes(machine, addr, bytes);
}

CallResult call_detect_drives(Machine& machine,
                              const std::unordered_map<std::string, uint16_t>& symbols) {
    const auto it = symbols.find("_mdr_detect_drives");
    if (it == symbols.end()) {
        throw std::runtime_error("symbol _mdr_detect_drives not found");
    }
    return call_function(machine, it->second);
}

CallResult call_dir(Machine& machine,
                    const std::unordered_map<std::string, uint16_t>& symbols,
                    uint8_t drive) {
    const auto it = symbols.find("_mdr_dir");
    if (it == symbols.end()) {
        throw std::runtime_error("symbol _mdr_dir not found");
    }

    std::fill(machine.mem.begin() + kDirAddr,
              machine.mem.begin() + kDirAddr + 32 * kDirEntrySize, 0);

    CallSetup setup;
    setup.a = drive;
    setup.d = static_cast<uint8_t>(kDirAddr >> 8);
    setup.e = static_cast<uint8_t>(kDirAddr & 0xFF);
    return call_function(machine, it->second, setup);
}

CallResult call_load(Machine& machine,
                     const std::unordered_map<std::string, uint16_t>& symbols,
                     uint8_t drive,
                     std::string_view name,
                     uint16_t dest_addr) {
    const auto it = symbols.find("_mdr_load");
    if (it == symbols.end()) {
        throw std::runtime_error("symbol _mdr_load not found");
    }

    write_name(machine, kNameAddr, name);

    CallSetup setup;
    setup.a = drive;
    setup.d = static_cast<uint8_t>(kNameAddr >> 8);
    setup.e = static_cast<uint8_t>(kNameAddr & 0xFF);
    setup.stack_args = {
        static_cast<uint8_t>(dest_addr & 0xFF),
        static_cast<uint8_t>(dest_addr >> 8),
    };
    return call_function(machine, it->second, setup);
}

CallResult call_save(Machine& machine,
                     const std::unordered_map<std::string, uint16_t>& symbols,
                     uint8_t drive,
                     std::string_view name,
                     std::span<const uint8_t> payload,
                     uint16_t src_addr) {
    const auto it = symbols.find("_mdr_save");
    if (it == symbols.end()) {
        throw std::runtime_error("symbol _mdr_save not found");
    }

    write_name(machine, kNameAddr, name);
    poke_bytes(machine, src_addr, payload);

    CallSetup setup;
    setup.a = drive;
    setup.d = static_cast<uint8_t>(kNameAddr >> 8);
    setup.e = static_cast<uint8_t>(kNameAddr & 0xFF);
    setup.stack_args = {
        static_cast<uint8_t>(src_addr & 0xFF),
        static_cast<uint8_t>(src_addr >> 8),
        static_cast<uint8_t>(payload.size() & 0xFF),
        static_cast<uint8_t>((payload.size() >> 8) & 0xFF),
    };
    return call_function(machine, it->second, setup);
}

CallResult call_format(Machine& machine,
                       const std::unordered_map<std::string, uint16_t>& symbols,
                       uint8_t drive,
                       std::string_view cart_name) {
    const auto it = symbols.find("_mdr_format");
    if (it == symbols.end()) {
        throw std::runtime_error("symbol _mdr_format not found");
    }

    write_name(machine, kNameAddr, cart_name);

    CallSetup setup;
    setup.a = drive;
    setup.d = static_cast<uint8_t>(kNameAddr >> 8);
    setup.e = static_cast<uint8_t>(kNameAddr & 0xFF);
    return call_function(machine, it->second, setup);
}

bool dir_contains(const Machine& machine,
                  std::string_view want_name,
                  uint8_t want_sectors,
                  uint16_t want_size,
                  uint8_t count) {
    for (uint8_t i = 0; i < count; ++i) {
        const uint16_t base = static_cast<uint16_t>(kDirAddr + i * kDirEntrySize);
        const auto name = trim_right_spaces(read_c_string(machine, base, 11));
        const uint8_t sectors = machine.mem[base + 11];
        const uint16_t size = static_cast<uint16_t>(machine.mem[base + 12]) |
                              (static_cast<uint16_t>(machine.mem[base + 13]) << 8);
        if (name == want_name && sectors == want_sectors && size == want_size) {
            return true;
        }
    }
    return false;
}

int find_sector_for_name(const microdrive::image_t& image, std::string_view name) {
    for (int sector = 0; sector < microdrive::k_num_sectors; ++sector) {
        const std::size_t base = static_cast<std::size_t>(sector) * microdrive::k_sector_size;
        const uint8_t flag = image.raw_byte(base + microdrive::k_off_record);
        if (flag == 0) {
            continue;
        }

        char field[microdrive::k_name_len];
        for (int i = 0; i < microdrive::k_name_len; ++i) {
            field[i] = static_cast<char>(
                image.raw_byte(base + microdrive::k_off_record + 4 + static_cast<std::size_t>(i)));
        }
        if (microdrive::name_match(field, name)) {
            return sector;
        }
    }

    return -1;
}

int count_invalid_used_records(const microdrive::image_t& image) {
    int invalid = 0;
    for (int sector = 0; sector < microdrive::k_num_sectors; ++sector) {
        const std::size_t base = static_cast<std::size_t>(sector) * microdrive::k_sector_size;
        const uint8_t flag = image.raw_byte(base + microdrive::k_off_record);
        if (flag == 0) {
            continue;
        }
        if (!image.record_checksum_ok(sector)) {
            invalid++;
        }
    }
    return invalid;
}

int run_detect_no_drive(const fs::path& root,
                        const std::unordered_map<std::string, uint16_t>& symbols) {
    Machine machine;
    load_rom(machine, root / kYosRomRelativePath);

    const CallResult result = call_detect_drives(machine, symbols);
    if (result.l != 0) {
        std::cerr << "detect-no-drive failed: expected 0, got " << unsigned(result.l) << "\n";
        return 1;
    }

    std::cout << "detect-no-drive: OK"
              << " steps=" << result.steps
              << " in=" << machine.ports.in_count
              << " out=" << machine.ports.out_count
              << "\n";
    return 0;
}

int run_detect_one_drive(const fs::path& root,
                         const std::unordered_map<std::string, uint16_t>& symbols) {
    Machine machine;
    load_rom(machine, root / kYosRomRelativePath);
    attach_blank_drive(machine, 0, "TEST");

    const CallResult result = call_detect_drives(machine, symbols);
    if (result.l != 1) {
        std::cerr << "detect-one-drive failed: expected 1, got " << unsigned(result.l) << "\n";
        return 1;
    }

    std::cout << "detect-one-drive: OK"
              << " steps=" << result.steps
              << " in=" << machine.ports.in_count
              << " out=" << machine.ports.out_count
              << "\n";
    return 0;
}

int run_dir_hello(const fs::path& root,
                  const std::unordered_map<std::string, uint16_t>& symbols) {
    const auto image = microdrive::image_t::load(root / "y/tests/microdrives/hello.mdr");
    const auto expected = image.get("hello.app");
    const uint8_t expected_sectors =
        static_cast<uint8_t>((expected.size() + microdrive::k_data_size - 1) / microdrive::k_data_size);

    Machine machine;
    load_rom(machine, root / kYosRomRelativePath);
    attach_drive(machine, 0, image);
    const CallResult result = call_dir(machine, symbols, 1);

    const auto count = result.l;
    const std::string name = trim_right_spaces(read_c_string(machine, kDirAddr, 11));
    const uint8_t sectors = machine.mem[kDirAddr + 11];
    const uint16_t size = static_cast<uint16_t>(machine.mem[kDirAddr + 12]) |
                          (static_cast<uint16_t>(machine.mem[kDirAddr + 13]) << 8);

    if (count != 1 || name != "hello.app" || sectors != expected_sectors || size != expected.size()) {
        std::cerr << "dir-hello failed: count=" << unsigned(count)
                  << " name=" << name
                  << " sectors=" << unsigned(sectors)
                  << " size=" << size
                  << " expected_size=" << expected.size()
                  << "\n";
        return 1;
    }

    std::cout << "dir-hello: OK"
              << " steps=" << result.steps
              << " name=" << name
              << " size=" << size
              << "\n";
    return 0;
}

int run_load_hello(const fs::path& root,
                   const std::unordered_map<std::string, uint16_t>& symbols) {
    const auto image = microdrive::image_t::load(root / "y/tests/microdrives/hello.mdr");
    const auto expected = image.get("hello.app");

    Machine machine;
    load_rom(machine, root / kYosRomRelativePath);
    attach_drive(machine, 0, image);

    std::fill(machine.mem.begin() + kLoadAddr,
              machine.mem.begin() + kLoadAddr + expected.size(), 0);
    const CallResult result = call_load(machine, symbols, 1, "hello.app", kLoadAddr);

    if (result.l != 0) {
        std::cerr << "load-hello failed: return=" << unsigned(result.l) << "\n";
        return 1;
    }

    if (!std::equal(expected.begin(), expected.end(), machine.mem.begin() + kLoadAddr)) {
        std::cerr << "load-hello failed: payload mismatch\n";
        return 1;
    }

    std::cout << "load-hello: OK"
              << " steps=" << result.steps
              << " bytes=" << expected.size()
              << "\n";
    return 0;
}

int run_save_roundtrip(const fs::path& root,
                       const std::unordered_map<std::string, uint16_t>& symbols,
                       std::string_view name,
                       std::size_t size) {
    Machine machine;
    load_rom(machine, root / kYosRomRelativePath);
    attach_blank_drive(machine, 0, "TEST");

    const auto payload = make_pattern(size);
    const uint8_t expected_sectors =
        static_cast<uint8_t>((size + microdrive::k_data_size - 1) / microdrive::k_data_size);

    const CallResult save_result = call_save(machine, symbols, 1, name, payload, kSaveAddr);
    if (save_result.l != 0) {
        std::cerr << "save-" << name << " failed: return=" << unsigned(save_result.l) << "\n";
        return 1;
    }

    const auto& drive = machine.ports.drives[0];
    if (!drive.image.has_value()) {
        std::cerr << "save-" << name << " failed: no drive image present\n";
        return 1;
    }

    const auto host_payload = drive.image->get(name);
    if (host_payload != payload) {
        std::cerr << "save-" << name << " failed: host image payload mismatch\n";
        return 1;
    }

    const CallResult dir_result = call_dir(machine, symbols, 1);
    if (dir_result.l == 0 || !dir_contains(machine, name, expected_sectors,
                                           static_cast<uint16_t>(size), dir_result.l)) {
        std::cerr << "save-" << name << " failed: dir missing expected entry"
                  << " count=" << unsigned(dir_result.l) << "\n";
        return 1;
    }

    std::fill(machine.mem.begin() + kLoadAddr,
              machine.mem.begin() + kLoadAddr + payload.size(), 0);
    const CallResult load_result = call_load(machine, symbols, 1, name, kLoadAddr);
    if (load_result.l != 0) {
        std::cerr << "save-" << name << " failed: reload returned "
                  << unsigned(load_result.l) << "\n";
        return 1;
    }

    if (!std::equal(payload.begin(), payload.end(), machine.mem.begin() + kLoadAddr)) {
        std::cerr << "save-" << name << " failed: reload payload mismatch\n";
        return 1;
    }

    std::cout << "save-" << name << ": OK"
              << " bytes=" << size
              << " sectors=" << unsigned(expected_sectors)
              << " save_steps=" << save_result.steps
              << " load_steps=" << load_result.steps
              << "\n";
    return 0;
}

int run_format_roundtrip(const fs::path& root,
                         const std::unordered_map<std::string, uint16_t>& symbols) {
    auto image = microdrive::image_t::load(root / "y/tests/microdrives/hello.mdr");
    image.put("f123", make_pattern(123));

    Machine machine;
    load_rom(machine, root / kYosRomRelativePath);
    attach_drive(machine, 0, std::move(image));

    const CallResult format_result = call_format(machine, symbols, 1, "FMT");
    if (format_result.l != 0) {
        std::cerr << "format-roundtrip failed: format return=" << unsigned(format_result.l) << "\n";
        return 1;
    }

    const CallResult dir_empty = call_dir(machine, symbols, 1);
    if (dir_empty.l != 0) {
        std::cerr << "format-roundtrip failed: expected empty dir, got "
                  << unsigned(dir_empty.l) << " entries\n";
        return 1;
    }

    const auto payload = make_pattern(123);
    const CallResult save_result = call_save(machine, symbols, 1, "tfmt", payload, kSaveAddr);
    if (save_result.l != 0) {
        std::cerr << "format-roundtrip failed: save return=" << unsigned(save_result.l) << "\n";
        return 1;
    }

    const CallResult dir_after = call_dir(machine, symbols, 1);
    if (!dir_contains(machine, "tfmt", 1, 123, dir_after.l)) {
        std::cerr << "format-roundtrip failed: formatted media not writable\n";
        return 1;
    }

    std::fill(machine.mem.begin() + kLoadAddr,
              machine.mem.begin() + kLoadAddr + payload.size(), 0);
    const CallResult load_result = call_load(machine, symbols, 1, "tfmt", kLoadAddr);
    if (load_result.l != 0 ||
        !std::equal(payload.begin(), payload.end(), machine.mem.begin() + kLoadAddr)) {
        std::cerr << "format-roundtrip failed: reload mismatch\n";
        return 1;
    }

    std::cout << "format-roundtrip: OK"
              << " format_steps=" << format_result.steps
              << " save_steps=" << save_result.steps
              << " load_steps=" << load_result.steps
              << "\n";
    return 0;
}

int run_format_drive3_roundtrip(const fs::path& root,
                                const std::unordered_map<std::string, uint16_t>& symbols) {
    Machine machine;
    load_rom(machine, root / kYosRomRelativePath);
    attach_blank_drive(machine, 0, "D1");
    attach_blank_drive(machine, 1, "D2");
    attach_blank_drive(machine, 2, "D3");

    const CallResult format_result = call_format(machine, symbols, 3, "D3FMT");
    if (format_result.l != 0) {
        std::cerr << "format-drive3 failed: format return=" << unsigned(format_result.l) << "\n";
        return 1;
    }

    const CallResult dir_empty = call_dir(machine, symbols, 3);
    if (dir_empty.l != 0) {
        std::cerr << "format-drive3 failed: expected empty dir, got "
                  << unsigned(dir_empty.l) << "\n";
        return 1;
    }

    const auto payload = make_pattern(123);
    const CallResult save_result = call_save(machine, symbols, 3, "d3tst", payload, kSaveAddr);
    if (save_result.l != 0) {
        std::cerr << "format-drive3 failed: save return=" << unsigned(save_result.l) << "\n";
        return 1;
    }

    const CallResult dir_after = call_dir(machine, symbols, 3);
    if (!dir_contains(machine, "d3tst", 1, 123, dir_after.l)) {
        std::cerr << "format-drive3 failed: missing saved file in dir"
                  << " save_ret=" << unsigned(save_result.l)
                  << " dir3_count=" << unsigned(dir_after.l);
        for (int drv = 1; drv <= 3; ++drv) {
            const CallResult d = call_dir(machine, symbols, static_cast<uint8_t>(drv));
            std::cerr << " dir" << drv << "=" << unsigned(d.l);
            if (dir_contains(machine, "d3tst", 1, 123, d.l)) {
                std::cerr << "(has d3tst)";
            }
        }
        std::cerr << "\n";
        return 1;
    }

    std::fill(machine.mem.begin() + kLoadAddr,
              machine.mem.begin() + kLoadAddr + payload.size(), 0);
    const CallResult load_result = call_load(machine, symbols, 3, "d3tst", kLoadAddr);
    if (load_result.l != 0 ||
        !std::equal(payload.begin(), payload.end(), machine.mem.begin() + kLoadAddr)) {
        std::cerr << "format-drive3 failed: load mismatch\n";
        return 1;
    }

    std::cout << "format-drive3: OK"
              << " format_steps=" << format_result.steps
              << " save_steps=" << save_result.steps
              << " load_steps=" << load_result.steps
              << "\n";
    return 0;
}

int run_save_duplicate(const fs::path& root,
                       const std::unordered_map<std::string, uint16_t>& symbols) {
    Machine machine;
    load_rom(machine, root / kYosRomRelativePath);
    attach_blank_drive(machine, 0, "TEST");

    const auto payload = make_pattern(123);
    const CallResult first = call_save(machine, symbols, 1, "dup1", payload, kSaveAddr);
    const auto after_first = machine.ports.drives[0].image->directory();
    const CallResult second = call_save(machine, symbols, 1, "dup1", payload, kSaveAddr + 0x200);
    const auto after_second = machine.ports.drives[0].image->directory();

    if (first.l != 0 || second.l != 2) {
        std::cerr << "save-duplicate failed: first=" << unsigned(first.l)
                  << " second=" << unsigned(second.l)
                  << " first_dir=" << after_first.size()
                  << " second_dir=" << after_second.size();
        for (const auto& entry : after_second) {
            std::cerr << " [" << entry.name << ":" << entry.bytes
                      << ":" << entry.sectors << "]";
        }
        std::cerr << "\n";
        return 1;
    }

    std::cout << "save-duplicate: OK"
              << " first_steps=" << first.steps
              << " second_steps=" << second.steps
              << "\n";
    return 0;
}

int run_save_full(const fs::path& root,
                  const std::unordered_map<std::string, uint16_t>& symbols) {
    Machine machine;
    load_rom(machine, root / kYosRomRelativePath);

    auto full = microdrive::image_t::create_blank("FULL");
    const auto payload =
        make_pattern(static_cast<std::size_t>(microdrive::k_num_sectors) * microdrive::k_data_size);
    full.put("full.bin", payload);
    attach_drive(machine, 0, std::move(full));

    const auto extra = make_pattern(16);
    const CallResult result = call_save(machine, symbols, 1, "extra", extra, kSaveAddr);
    if (result.l != 1) {
        std::cerr << "save-full failed: expected 1, got " << unsigned(result.l) << "\n";
        return 1;
    }

    std::cout << "save-full: OK"
              << " steps=" << result.steps
              << "\n";
    return 0;
}

int run_save_sequence(const fs::path& root,
                      const std::unordered_map<std::string, uint16_t>& symbols) {
    struct Case {
        std::string name;
        std::size_t size;
        uint16_t src_addr;
    };

    const std::array<Case, 3> cases {{
        {"s123", 123, static_cast<uint16_t>(kSaveAddr + 0x000)},
        {"s512", 512, static_cast<uint16_t>(kSaveAddr + 0x300)},
        {"s777", 777, static_cast<uint16_t>(kSaveAddr + 0x700)},
    }};

    Machine machine;
    load_rom(machine, root / kYosRomRelativePath);
    attach_blank_drive(machine, 0, "SEQ");

    for (const auto& tc : cases) {
        const auto payload = make_pattern(tc.size);
        const CallResult save_result = call_save(machine, symbols, 1, tc.name, payload, tc.src_addr);
        if (save_result.l != 0) {
            std::cerr << "save-sequence failed on save " << tc.name
                      << ": return=" << unsigned(save_result.l) << "\n";
            return 1;
        }
    }

    const CallResult dir_result = call_dir(machine, symbols, 1);
    if (dir_result.l < 3) {
        std::cerr << "save-sequence failed: dir count=" << unsigned(dir_result.l) << "\n";
        return 1;
    }

    for (const auto& tc : cases) {
        const uint8_t expected_sectors =
            static_cast<uint8_t>((tc.size + microdrive::k_data_size - 1) / microdrive::k_data_size);
        if (!dir_contains(machine, tc.name, expected_sectors, static_cast<uint16_t>(tc.size),
                          dir_result.l)) {
            std::cerr << "save-sequence failed: missing dir entry " << tc.name << "\n";
            return 1;
        }
    }

    for (const auto& tc : cases) {
        const auto expected = make_pattern(tc.size);
        std::fill(machine.mem.begin() + kLoadAddr,
                  machine.mem.begin() + kLoadAddr + tc.size, 0);

        const CallResult load_result = call_load(machine, symbols, 1, tc.name, kLoadAddr);
        if (load_result.l != 0) {
            std::cerr << "save-sequence failed on load " << tc.name
                      << ": return=" << unsigned(load_result.l) << "\n";
            return 1;
        }

        if (!std::equal(expected.begin(), expected.end(), machine.mem.begin() + kLoadAddr)) {
            std::cerr << "save-sequence failed: payload mismatch " << tc.name << "\n";
            return 1;
        }
    }

    const auto& drive = machine.ports.drives[0];
    if (!drive.image.has_value()) {
        std::cerr << "save-sequence failed: drive image missing\n";
        return 1;
    }

    const fs::path persist_path = root / "build/mdr-emu/sequence.mdr";
    fs::create_directories(persist_path.parent_path());
    drive.image->save(persist_path);
    const auto reloaded = microdrive::image_t::load(persist_path);
    for (const auto& tc : cases) {
        if (reloaded.get(tc.name) != make_pattern(tc.size)) {
            std::cerr << "save-sequence failed: persisted image mismatch " << tc.name << "\n";
            return 1;
        }
    }

    std::cout << "save-sequence: OK"
              << " dir_count=" << unsigned(dir_result.l)
              << " persisted=" << persist_path.string()
              << "\n";
    return 0;
}

int run_load_missing(const fs::path& root,
                     const std::unordered_map<std::string, uint16_t>& symbols) {
    Machine machine;
    load_rom(machine, root / kYosRomRelativePath);
    attach_blank_drive(machine, 0, "MISS");

    std::fill(machine.mem.begin() + kLoadAddr, machine.mem.begin() + kLoadAddr + 256, 0xA5);
    const CallResult result = call_load(machine, symbols, 1, "notfound", kLoadAddr);
    if (result.l != 1) {
        std::cerr << "load-missing failed: expected 1, got " << unsigned(result.l) << "\n";
        return 1;
    }

    std::cout << "load-missing: OK"
              << " steps=" << result.steps
              << "\n";
    return 0;
}

int run_save_zero_length(const fs::path& root,
                         const std::unordered_map<std::string, uint16_t>& symbols) {
    Machine machine;
    load_rom(machine, root / kYosRomRelativePath);
    attach_blank_drive(machine, 0, "ZERO");

    const std::vector<uint8_t> payload;
    const CallResult result = call_save(machine, symbols, 1, "zero", payload, kSaveAddr);
    if (result.l != 3) {
        std::cerr << "save-zero failed: expected 3, got " << unsigned(result.l) << "\n";
        return 1;
    }

    const CallResult dir_result = call_dir(machine, symbols, 1);
    if (dir_result.l != 0) {
        std::cerr << "save-zero failed: dir count changed to " << unsigned(dir_result.l) << "\n";
        return 1;
    }

    std::cout << "save-zero: OK"
              << " steps=" << result.steps
              << "\n";
    return 0;
}

int run_save_fragmented(const fs::path& root,
                        const std::unordered_map<std::string, uint16_t>& symbols) {
    auto image = microdrive::image_t::create_blank("FRAG");
    image.put("a111", make_pattern(111));
    image.put("b222", make_pattern(222));
    image.put("c333", make_pattern(333));
    image.remove("b222");

    Machine machine;
    load_rom(machine, root / kYosRomRelativePath);
    attach_drive(machine, 0, std::move(image));

    const auto payload = make_pattern(777);
    const CallResult save_result = call_save(machine, symbols, 1, "frag777", payload, kSaveAddr);
    if (save_result.l != 0) {
        std::cerr << "save-fragmented failed: save return=" << unsigned(save_result.l) << "\n";
        return 1;
    }

    std::fill(machine.mem.begin() + kLoadAddr,
              machine.mem.begin() + kLoadAddr + payload.size(), 0);
    const CallResult load_result = call_load(machine, symbols, 1, "frag777", kLoadAddr);
    if (load_result.l != 0) {
        std::cerr << "save-fragmented failed: load return=" << unsigned(load_result.l) << "\n";
        return 1;
    }
    if (!std::equal(payload.begin(), payload.end(), machine.mem.begin() + kLoadAddr)) {
        std::cerr << "save-fragmented failed: payload mismatch\n";
        return 1;
    }

    std::cout << "save-fragmented: OK"
              << " save_steps=" << save_result.steps
              << " load_steps=" << load_result.steps
              << "\n";
    return 0;
}

int run_dir_capacity(const fs::path& root,
                     const std::unordered_map<std::string, uint16_t>& symbols) {
    auto image = microdrive::image_t::create_blank("MANY");
    for (int i = 0; i < 40; ++i) {
        std::ostringstream name;
        name << 'f' << (i / 10) << (i % 10);
        image.put(name.str(), make_pattern(16));
    }

    Machine machine;
    load_rom(machine, root / kYosRomRelativePath);
    attach_drive(machine, 0, std::move(image));

    const CallResult result = call_dir(machine, symbols, 1);
    if (result.l != 32) {
        std::cerr << "dir-capacity failed: expected 32, got " << unsigned(result.l) << "\n";
        return 1;
    }
    if (!dir_contains(machine, "f00", 1, 16, result.l)) {
        std::cerr << "dir-capacity failed: expected f00 in listing\n";
        return 1;
    }

    std::cout << "dir-capacity: OK"
              << " count=" << unsigned(result.l)
              << "\n";
    return 0;
}

int run_corrupt_checksum(const fs::path& root,
                         const std::unordered_map<std::string, uint16_t>& symbols) {
    auto image = microdrive::image_t::load(root / "y/tests/microdrives/hello.mdr");
    const int sector = find_sector_for_name(image, "hello.app");
    if (sector < 0) {
        std::cerr << "corrupt-checksum failed: hello.app not found in source image\n";
        return 1;
    }

    const std::size_t chk_index = static_cast<std::size_t>(sector) * microdrive::k_sector_size +
                                  microdrive::k_off_record + 14;
    image.set_raw_byte(chk_index, static_cast<uint8_t>(image.raw_byte(chk_index) ^ 0x5A));

    Machine machine;
    load_rom(machine, root / kYosRomRelativePath);
    attach_drive(machine, 0, std::move(image));

    const CallResult dir_result = call_dir(machine, symbols, 1);
    if (dir_contains(machine, "hello.app", 1, 233, dir_result.l)) {
        std::cerr << "corrupt-checksum failed: corrupted entry still visible in dir\n";
        return 1;
    }

    const CallResult load_result = call_load(machine, symbols, 1, "hello.app", kLoadAddr);
    if (load_result.l != 1) {
        std::cerr << "corrupt-checksum failed: expected load miss, got "
                  << unsigned(load_result.l) << "\n";
        return 1;
    }

    std::cout << "corrupt-checksum: OK"
              << " dir_count=" << unsigned(dir_result.l)
              << "\n";
    return 0;
}

int run_repro_strict(const fs::path& root,
                     const std::unordered_map<std::string, uint16_t>& symbols) {
    Machine machine;
    load_rom(machine, root / kYosRomRelativePath);
    auto base_image = microdrive::image_t::load(
        root / "bin/z/z80/spectrum/bin/mdr/mdrstep.mdr");
    (void)base_image.remove("t123");
    std::vector<uint8_t> before;
    before.reserve(microdrive::k_image_size);
    for (int i = 0; i < microdrive::k_image_size; ++i) {
        before.push_back(base_image.raw_byte(static_cast<std::size_t>(i)));
    }
    attach_drive(machine, 0, base_image);

    const auto payload = make_pattern(123);
    CallResult save_result {};
    g_write_events.clear();
    g_trace_writes = true;

    try {
        save_result = call_save(machine, symbols, 1, "t123", payload, kSaveAddr);
    } catch (const std::exception& e) {
        g_trace_writes = false;
        std::cerr << "strict-repro failed: save timeout/error: " << e.what() << "\n";
        return 1;
    }
    g_trace_writes = false;

    if (!machine.ports.drives[0].image.has_value()) {
        std::cerr << "strict-repro: drive image missing after save\n";
        return 1;
    }

    const auto& image = *machine.ports.drives[0].image;
    const auto host_dir = image.directory();
    bool has_t123 = false;
    for (const auto& entry : host_dir) {
        if (entry.name == "t123") {
            has_t123 = true;
            break;
        }
    }
    const int invalid_records = count_invalid_used_records(image);
    const CallResult detect_after_save = call_detect_drives(machine, symbols);
    const bool detect_ok = detect_after_save.l >= 1;
    const CallResult dir_after_save = call_dir(machine, symbols, 1);
    const bool dir_has_t123 = dir_contains(machine, "t123", 1, 123, dir_after_save.l);
    const CallResult load_result = call_load(machine, symbols, 1, "t123", kLoadAddr);
    const bool load_ok = (load_result.l == 0);
    const bool payload_ok = load_ok &&
                            std::equal(payload.begin(), payload.end(),
                                       machine.mem.begin() + kLoadAddr);
    int changed_bytes = 0;
    int changed_sectors = 0;
    for (int sector = 0; sector < microdrive::k_num_sectors; ++sector) {
        bool sector_changed = false;
        const std::size_t base = static_cast<std::size_t>(sector) * microdrive::k_sector_size;
        for (int i = 0; i < microdrive::k_sector_size; ++i) {
            const std::size_t idx = base + static_cast<std::size_t>(i);
            if (before[idx] != image.raw_byte(idx)) {
                changed_bytes++;
                sector_changed = true;
            }
        }
        if (sector_changed) {
            changed_sectors++;
        }
    }

    const bool pass = (save_result.l == 0) &&
                      has_t123 &&
                      (invalid_records == 0) &&
                      detect_ok &&
                      dir_has_t123 &&
                      load_ok &&
                      payload_ok;

    if (pass) {
        std::cout << "strict-repro: OK"
                  << " save_steps=" << save_result.steps
                  << " detect=" << unsigned(detect_after_save.l)
                  << " dir_after=" << unsigned(dir_after_save.l)
                  << " load_steps=" << load_result.steps
                  << " dir_count=" << host_dir.size()
                  << " changed_sectors=" << changed_sectors
                  << " changed_bytes=" << changed_bytes
                  << "\n";
        return 0;
    }

    std::cerr << "strict-repro failed:"
              << " save_ret=" << unsigned(save_result.l)
              << " has_t123=" << (has_t123 ? "yes" : "no")
              << " invalid_used_records=" << invalid_records
              << " detect_after=" << unsigned(detect_after_save.l)
              << " dir_after=" << unsigned(dir_after_save.l)
              << " dir_has_t123=" << (dir_has_t123 ? "yes" : "no")
              << " load_ret=" << unsigned(load_result.l)
              << " payload_ok=" << (payload_ok ? "yes" : "no")
              << " dir_count=" << host_dir.size()
              << "\n";
    for (const auto& entry : host_dir) {
        std::cerr << "  dir: " << entry.name
                  << " bytes=" << entry.bytes
                  << " sectors=" << entry.sectors
                  << "\n";
    }
    std::cerr << "  writes=" << g_write_events.size() << "\n";
    for (std::size_t i = 0; i < g_write_events.size() && i < 24; ++i) {
        const auto& ev = g_write_events[i];
        const int sec = ev.pos / microdrive::k_sector_size;
        const int off = ev.pos % microdrive::k_sector_size;
        std::cerr << "  w" << i
                  << " sec=" << (sec + 1)
                  << " off=" << off
                  << " ctrl=0x" << std::hex << std::uppercase << unsigned(ev.ctrl)
                  << " val=0x" << unsigned(ev.value) << std::dec
                  << "\n";
    }
    return 1;
}

int run_size_report(const fs::path& root) {
    struct Entry {
        std::string label;
        fs::path path;
    };

    const std::array<Entry, 6> entries {{
        {"mdr_common.rel", root / "build/yos/mdr_common.rel"},
        {"mdr_detect_drives.rel", root / "build/yos/mdr_detect_drives.rel"},
        {"mdr_dir.rel", root / "build/yos/mdr_dir.rel"},
        {"mdr_load.rel", root / "build/yos/mdr_load.rel"},
        {"mdr_save.rel", root / "build/yos/mdr_save.rel"},
        {"yos.rom", root / kYosRomRelativePath},
    }};

    std::uintmax_t total_rel = 0;
    for (const auto& entry : entries) {
        if (!fs::exists(entry.path)) {
            throw std::runtime_error("size report missing file: " + entry.path.string());
        }
    }

    std::cout << "size-report:\n";
    for (const auto& entry : entries) {
        const auto bytes = fs::file_size(entry.path);
        if (entry.path.extension() == ".rel") {
            total_rel += bytes;
        }
        std::cout << "  " << entry.label << ": " << bytes << " bytes\n";
    }
    std::cout << "  mdr-rel-total: " << total_rel << " bytes\n";
    return 0;
}

int run_smoke_once(const fs::path& root, bool print_header = false, int iteration = 1) {
    const auto symbols = load_symbols(root / kYosCdbRelativePath);
    if (print_header) {
        std::cout << "smoke-iteration: " << iteration << "\n";
    }

    if (run_detect_no_drive(root, symbols)) return 1;
    if (run_detect_one_drive(root, symbols)) return 1;
    if (run_dir_hello(root, symbols)) return 1;
    if (run_load_hello(root, symbols)) return 1;
    if (run_save_roundtrip(root, symbols, "t123", 123)) return 1;
    if (run_save_roundtrip(root, symbols, "t512", 512)) return 1;
    if (run_save_roundtrip(root, symbols, "t777", 777)) return 1;
    if (run_format_roundtrip(root, symbols)) return 1;
    if (run_format_drive3_roundtrip(root, symbols)) return 1;
    if (run_save_duplicate(root, symbols)) return 1;
    if (run_save_full(root, symbols)) return 1;
    if (run_save_sequence(root, symbols)) return 1;
    if (run_load_missing(root, symbols)) return 1;
    if (run_save_zero_length(root, symbols)) return 1;
    if (run_save_fragmented(root, symbols)) return 1;
    if (run_dir_capacity(root, symbols)) return 1;
    if (run_corrupt_checksum(root, symbols)) return 1;

    std::cout << "smoke: all checks passed";
    if (print_header) {
        std::cout << " (iteration " << iteration << ")";
    }
    std::cout << "\n";
    return 0;
}

int run_stress(const fs::path& root, int iterations) {
    if (iterations < 1) {
        throw std::runtime_error("stress iterations must be >= 1");
    }

    for (int i = 1; i <= iterations; ++i) {
        if (run_smoke_once(root, true, i)) {
            std::cerr << "stress failed at iteration " << i << "\n";
            return 1;
        }
    }

    std::cout << "stress: all iterations passed (" << iterations << ")\n";
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const fs::path root =
            fs::canonical(fs::path(argv[0])).parent_path().parent_path().parent_path();

        if (argc < 2) {
            std::cerr << "usage: mdr_emu smoke | repro-strict | stress [iterations] | size\n";
            return 1;
        }

        const std::string_view cmd(argv[1]);
        if (cmd == "smoke") {
            g_strict_timing = false;
            return run_smoke_once(root);
        }
        if (cmd == "repro-strict") {
            g_strict_timing = true;
            std::cout << "mode: strict-timing\n";
            const auto symbols = load_symbols(root / kYosCdbRelativePath);
            return run_repro_strict(root, symbols);
        }
        if (cmd == "stress") {
            g_strict_timing = false;
            int iterations = 10;
            if (argc >= 3) {
                std::istringstream iss(argv[2]);
                if (!(iss >> iterations)) {
                    throw std::runtime_error("invalid stress iteration count");
                }
            }
            return run_stress(root, iterations);
        }
        if (cmd == "size") {
            return run_size_report(root);
        }

        std::cerr << "unknown command: " << cmd << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 2;
    }
}
