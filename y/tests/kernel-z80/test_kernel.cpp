#include <xz80/xz80.h>
#include "mock_filesystem.h"
#include "test_libraries.h"
#include "test_thread_safety.h"

#include <array>
#include <cstdint>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct memory final : xz80::IMemory {
    std::array<std::uint8_t, 65536> bytes{};
    unsigned rom_writes = 0;
    std::function<void(std::uint16_t, bool)> observe;

    std::uint8_t read(std::uint16_t address) const noexcept override {
        if (observe) observe(address, false);
        return bytes[address];
    }

    void write(std::uint16_t address, std::uint8_t value) noexcept override {
        if (observe) observe(address, true);
        if (address < 0x4000) {
            ++rom_writes;
            return;
        }
        bytes[address] = value;
    }

    std::uint16_t word(std::uint16_t address) const {
        return bytes[address] |
            (std::uint16_t(bytes[std::uint16_t(address + 1)]) << 8);
    }

    void word(std::uint16_t address, std::uint16_t value) {
        bytes[address] = value & 0xff;
        bytes[std::uint16_t(address + 1)] = value >> 8;
    }
};

struct ports final : xz80::IPorts {
    std::uint8_t mouse_buttons = 0xff;
    std::uint8_t mouse_x = 0;
    std::uint8_t mouse_y = 0;
    std::uint16_t keyboard_address = 0;
    std::uint8_t keyboard_value = 0xff;

    std::uint8_t in(std::uint16_t address) noexcept override {
        if (address == 0xfadf) return mouse_buttons;
        if (address == 0xfbdf) return mouse_x;
        if (address == 0xffdf) return mouse_y;
        if (address == keyboard_address) return keyboard_value;
        return 0xff;
    }
    void out(std::uint16_t, std::uint8_t) noexcept override {}
};

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

std::map<std::string, std::uint16_t> read_symbols(const char* path) {
    std::ifstream input(path);
    require(bool(input), std::string("cannot read map ") + path);
    std::map<std::string, std::uint16_t> result;
    std::string line;
    while (std::getline(input, line)) {
        std::istringstream fields(line);
        std::string address, name;
        if (fields >> address >> name && address.size() == 8 &&
            address.find_first_not_of("0123456789abcdefABCDEF") ==
                std::string::npos)
            result[name] = std::stoul(address, nullptr, 16);
    }
    return result;
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 6) {
        std::cerr << "usage: test_kernel ROM MAP SHELL.SYS LIB.SVC LIB.MAP\n";
        return 2;
    }

    memory mem;
    ports io;
    xz80::cpu cpu(mem, io);
    std::ifstream rom(argv[1], std::ios::binary);
    rom.read(reinterpret_cast<char*>(mem.bytes.data()), 0x4000);
    require(rom.gcount() == 0x4000, "kernel ROM is not 16 KiB");

    const auto symbols = read_symbols(argv[2]);
    const auto sym = [&](const std::string& name) {
        const auto found = symbols.find(name);
        require(found != symbols.end(), "missing symbol " + name);
        return found->second;
    };
    mock_filesystem files{mem, cpu, sym("__zx_esx_gate_9a"),
                          sym("__zx_esx_gate_9d"),
                          sym("__zx_esx_gate_9b")};
    files.write_gate = sym("__zx_esx_gate_9e");
    files.seek_gate = sym("__zx_esx_gate_9f");
    files.position_gate = sym("__zx_esx_gate_a0");
    files.status_gate = sym("__zx_esx_gate_a1");
    files.sync_gate = sym("__zx_esx_gate_9c");
    bool booting = true;
    const auto run_until = [&](auto stop, unsigned limit,
                               const std::string& operation) {
        for (unsigned step = 0; step < limit; ++step) {
            if (stop()) return;
            // Test the exact production ROM, with disk boot deferred until
            // the RAM-gate fixture is populated below. Never patch ROM bytes.
            if (booting && cpu.pc() == sym("_boot_shell")) {
                auto state = cpu.snapshot();
                state.pc = mem.word(state.sp);
                state.sp += 2;
                state.de = 0;
                cpu.restore(state);
                continue;
            }
            if (!files.step()) cpu.step();
        }
        throw std::runtime_error(operation + " reached emulator step limit at PC " +
                                 std::to_string(cpu.pc()));
    };

    cpu.reset();
    auto state = cpu.snapshot();
    state.pc = 0;
    state.sp = 0xffff;
    cpu.restore(state);
    run_until([&] { return cpu.halted(); }, 200000, "boot");
    booting = false;

    const auto vectors = sym("__sys_vec_tbl");
    constexpr std::array<std::uint16_t, 4> restart_addresses = {
        0x0018, 0x0020, 0x0028, 0x0030};
    for (std::size_t index = 0; index < restart_addresses.size(); ++index) {
        const auto address = restart_addresses[index];
        require(mem.bytes[address] == 0xc3,
                "fixed RST entry is not JP");
        require(mem.word(address + 1) == vectors + 3 * (index + 2),
                "fixed RST entry targets the wrong RAM vector");
    }
    require(mem.bytes[0x0010] == 0xc9,
            "RST10 does not preserve esxDOS's immediate boot return");
    require(mem.bytes[0x0000] == 0xf3 && mem.bytes[0x0001] == 0xaf &&
                mem.bytes[0x0002] == 0xc3,
            "reset does not preserve divIDE's DI/XOR A return signature");
    require(mem.bytes[0x0008] == 0x2a,
            "RST8 does not preserve esxDOS's delayed LD HL opcode");
    for (std::uint16_t address = 0x000b; address != 0x0010; ++address) {
        require(mem.bytes[address] == 0x00,
                "RST8 reserved tail is not NOP-filled");
    }
    require(mem.bytes[0x0038] == 0xf5,
            "RST38 does not preserve divIDE's IRQ entry PUSH AF");
    require(mem.bytes[0x0039] == 0xf1 && mem.bytes[0x003a] == 0xfb &&
                mem.bytes[0x003b] == 0xed && mem.bytes[0x003c] == 0x4d,
            "RST38 does not preserve esxDOS's exact IM1 return");
    require(mem.bytes[0x0066] == 0xf5,
            "NMI does not preserve divIDE's delayed PUSH AF entry");
    for (std::uint16_t address = 0x0067; address != 0x007b; ++address) {
        require(mem.bytes[address] == 0x00,
                "NMI reserved tail is not NOP-filled");
    }
    require(mem.bytes[0x007b] == 0x7e && mem.bytes[0x007c] == 0xc9,
            "fixed esxDOS base-ROM byte reader is missing");
    require(mem.bytes[vectors + 6] == 0xc3, "RST18 vector is not JP");
    require(mem.word(vectors + 7) == sym("_svc_query_rst18"),
            "RST18 service vector is wrong");
    require(mem.bytes[vectors + 18] == 0xc3, "RST38 vector is not JP");
    require(mem.word(vectors + 19) == sym("__sys_reti"),
            "unused RST38 RAM vector is not the default return");
    require(mem.word(sym("__im2_vector")) == sym("__thread_robin"),
            "IM2 scheduler vector is wrong");

    const auto table = sym("__yos");
    constexpr std::array<const char*, 49> yos_api = {
        "_yos_version", "__yos_malloc", "__yos_free", "__clock",
        "_enter_critical_section", "_leave_critical_section",
        "__yos_install_timer", "_tmr_uninstall",
        "_evt_create", "_evt_destroy", "_evt_set",
        "_thread_create", "_thread_exit", "_thread_suspend",
        "_thread_resume", "_process_start", "_process_exit",
        "__svc_query", "_svc_register", "_svc_unregister",
        "_sys_vec_get", "_sys_vec_set",
        "_kbd_read", "_mouse_calibrate", "_mouse_read",
        "__errno_value",
        "_open", "_close", "_read", "_write", "_lseek", "_fsync",
        "_unlink", "_rename", "_chdir", "_getcwd", "_mkdir", "_rmdir",
        "_stat", "_fstat", "_opendir", "_readdir", "_rewinddir",
        "_closedir", "_enumerate_disks", "_process_load",
        "_process_last_error", "_library_load", "__yos_shrink"
    };
    for (std::size_t slot = 0; slot < yos_api.size(); ++slot) {
        require(mem.word(table + 2 * slot) == sym(yos_api[slot]),
                "YOS API slot " + std::to_string(slot) +
                    " does not match " + yos_api[slot]);
    }
    constexpr std::array<std::uint8_t, 19> esxdos_services = {
        0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 0xa0,
        0xa1, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xb0,
        0xa3, 0xa4, 0xa7, 0x84};
    const auto gates = sym("__zx_esx_gates_start");
    for (std::size_t gate = 0; gate < esxdos_services.size(); ++gate) {
        const auto address = std::uint16_t(gates + 3 * gate);
        require(mem.bytes[address] == 0xcf &&
                    mem.bytes[address + 1] == esxdos_services[gate] &&
                    mem.bytes[address + 2] == 0xc9,
                "esxDOS RAM gate " + std::to_string(gate) +
                    " was not initialized");
    }
    require(mem.word(sym("__svc_first")) != 0,
            "YOS service was not registered");
    unsigned kernel_timers = 0;
    bool has_clock_timer = false;
    bool has_keyboard_timer = false;
    bool has_mouse_timer = false;
    for (auto timer = mem.word(sym("__tmr_first")); timer != 0;
         timer = mem.word(timer)) {
        require(++kernel_timers <= 3, "unexpected kernel timer chain");
        const auto hook = mem.word(timer + 4);
        has_clock_timer |= hook == sym("__clock_tick");
        has_keyboard_timer |= hook == sym("__kbd_scan");
        has_mouse_timer |= hook == sym("__mouse_scan");
        require(mem.word(timer + 6) == 0 && mem.word(timer + 8) == 0,
                "kernel input/clock timer is not frame-periodic");
    }
    require(kernel_timers == 3 && has_clock_timer && has_keyboard_timer &&
                has_mouse_timer,
            "clock, keyboard and mouse timers were not all installed");

    // Nested sections preserve every register and the incoming IFF state,
    // including callers already inside an interrupt handler (depth zero).
    for (const bool enabled : {false, true}) {
        auto initial = cpu.snapshot();
        initial.halted = false;
        initial.iff1 = initial.iff2 = enabled;
        initial.af = 0xa5d7;
        initial.bc = 0x2345;
        initial.de = 0x3456;
        initial.hl = 0x4567;
        initial.ix = 0x5678;
        initial.iy = 0x6789;
        cpu.restore(initial);
        const auto critical = [&](const char* name) {
            auto state = cpu.snapshot();
            state.pc = sym(name);
            state.sp = 0xeffe;
            mem.word(state.sp, 0x4100);
            cpu.restore(state);
            run_until([&] { return cpu.pc() == 0x4100; }, 200,
                      "critical-section register preservation");
            state = cpu.snapshot();
            require(state.af == initial.af && state.bc == initial.bc &&
                        state.de == initial.de && state.hl == initial.hl &&
                        state.ix == initial.ix && state.iy == initial.iy,
                    "critical section changed a register or flags");
        };
        critical("_leave_critical_section"); // unmatched leave must not EI
        require(cpu.snapshot().iff1 == enabled, "unmatched leave changed IFF");
        critical("_enter_critical_section");
        critical("_enter_critical_section");
        require(!cpu.interrupt(0xff), "nested critical section accepted IRQ");
        critical("_leave_critical_section");
        require(!cpu.snapshot().iff1, "inner leave enabled IRQ");
        critical("_leave_critical_section");
        require(cpu.snapshot().iff1 == enabled &&
                    cpu.snapshot().iff2 == enabled &&
                    !mem.bytes[sym("__interrupt_refcount")],
                "outer leave did not restore IFF or nesting depth");
    }

    // NMOS LD A,I/IRQ boundary: the scheduler repair must change only the
    // saved P/V bit, and only at the exact critical-entry sample address.
    for (const int delta : {0, 1, 256, -1}) {
        const auto pc = std::uint16_t(sym("__critical_iff_sampled") + delta);
        auto before = cpu.snapshot();
        before.halted = false;
        before.pc = sym("__critical_iff_repair");
        before.sp = 0xeff0;
        before.bc = 0x2345;
        before.de = 0x3456;
        before.ix = 0x5678;
        before.iy = 0x6789;
        mem.word(0xeff0, 0x4100); // helper return
        mem.word(0xeff2, 0x4567); // interrupted HL
        mem.word(0xeff4, 0xa5d3); // interrupted AF with P/V cleared
        mem.word(0xeff6, pc);     // interrupted PC
        cpu.restore(before);
        run_until([&] { return cpu.pc() == 0x4100; }, 100,
                  "critical IFF sample repair");
        const auto after = cpu.snapshot();
        require(mem.word(0xeff4) == (delta == 0 ? 0xa5d7 : 0xa5d3) &&
                    mem.word(0xeff2) == 0x4567 && mem.word(0xeff6) == pc &&
                    after.sp == 0xeff2 && after.bc == before.bc &&
                    after.de == before.de && after.ix == before.ix &&
                    after.iy == before.iy,
                "IFF repair changed unrelated state or matched wrong PC");
    }

    std::uint16_t kernel_hl = 0;
    const auto call_kernel = [&](std::uint16_t function, std::uint16_t hl,
                                 std::uint16_t de,
                                 const std::string& operation,
                                 std::uint8_t a = 0,
                                 std::initializer_list<std::uint8_t>
                                     stack_arguments = {},
                                 std::uint16_t bc = 0,
                                 bool caller_cleans = false) {
        constexpr std::uint16_t direct_sp = 0xf000;
        constexpr std::uint16_t direct_return = 0x4100;
        mem.word(direct_sp - 2, direct_return);
        std::uint16_t argument_address = direct_sp;
        for (const auto byte : stack_arguments)
            mem.bytes[argument_address++] = byte;
        auto direct = cpu.snapshot();
        direct.halted = false;
        direct.pc = function;
        direct.sp = direct_sp - 2;
        direct.hl = hl;
        direct.de = de;
        direct.bc = bc;
        direct.af = std::uint16_t(a) << 8;
        direct.ix = 0xa55a;
        direct.iy = 0x5aa5;
        cpu.restore(direct);
        run_until([&] { return cpu.pc() == direct_return; }, 500000,
                  operation);
        direct = cpu.snapshot();
        require(direct.sp == direct_sp +
                    (caller_cleans ? 0 : stack_arguments.size()) &&
                    direct.ix == 0xa55a &&
                    direct.iy == 0x5aa5,
                operation + " violated its ABI");
        kernel_hl = direct.hl;
        return direct.de;
    };

    std::ifstream shell_file(argv[3], std::ios::binary);
    const std::vector<std::uint8_t> shell{
        std::istreambuf_iterator<char>(shell_file),
        std::istreambuf_iterator<char>()};
    std::ifstream library_file(argv[4], std::ios::binary);
    const std::vector<std::uint8_t> library_image{
        std::istreambuf_iterator<char>(library_file),
        std::istreambuf_iterator<char>()};
    const auto library_symbols = read_symbols(argv[5]);
    require(!library_image.empty() && library_symbols.contains("_interface"),
            "missing packaged library fixture");
    files.files["shell.sys"] = shell;
    files.files["shelllib.svc"] = library_image;
    require(shell.size() >= 76 && shell[0] == 'X' && shell[1] == 'P' &&
                shell[2] == 'R' && shell[3] == 'G',
            "dummy shell is not an XPRG image");
    require(shell[30] == 1 && shell[31] == 0 &&
                library_image[30] == 1 && library_image[31] == 0 &&
                call_kernel(sym("_yos_version"), 0, 0, "ABI version") == 1,
            "ROM and packaged images do not use ABI 1");
    const auto payload_offset = std::uint16_t(shell[10] | (shell[11] << 8));
    const auto payload_size = std::uint16_t(shell[12] | (shell[13] << 8));
    require(payload_offset + payload_size == shell.size(),
            "dummy shell payload length is inconsistent");
    constexpr std::uint16_t shell_image = 0x8000;
    for (std::uint16_t i = 0; i < payload_size; ++i)
        mem.bytes[shell_image + i] = shell[payload_offset + i];
    const auto crc_high = call_kernel(sym("__crc32"), shell_image, 0,
                                      "XPRG payload CRC", 0, {}, payload_size);
    require(kernel_hl == std::uint16_t(shell[16] | (shell[17] << 8)) &&
                crc_high == std::uint16_t(shell[18] | (shell[19] << 8)),
            "ROM CRC-32 disagrees with xprog");
    constexpr std::uint16_t relocated_image = 0x9000;
    for (std::uint16_t i = 0; i < payload_size; ++i)
        mem.bytes[relocated_image + i] = shell[payload_offset + i];
    // XL v2: 12-byte header, code, then the relocation table.
    require(shell[payload_offset + 2] == 2, "dummy shell is not XL version 2");
    const auto code_size = mem.word(relocated_image + 6);
    const auto relocation_count = mem.word(relocated_image + 8);
    require(12 + code_size + 4 * relocation_count == payload_size,
            "dummy shell XL layout is inconsistent");
    const auto code_base = call_kernel(sym("__process_relocate"),
                                       relocated_image, payload_size,
                                       "dummy shell relocation");
    const auto relocated_code = code_base;
    require(code_base != 0 && kernel_hl == code_base,
            "XL relocator returned the wrong code base: " +
                std::to_string(code_base));
    require(code_base == relocated_image + 12,
            "XL relocator did not reuse the existing code buffer");
    require(cpu.snapshot().bc == code_size,
            "XL relocator did not return the code size");
    const auto relocation_table = std::uint16_t(relocated_image + 12 +
                                                code_size);
    for (std::uint16_t index = 0; index < relocation_count; ++index) {
        const auto record = std::uint16_t(relocation_table + 4 * index);
        const auto offset = mem.word(record);
        const auto width = mem.bytes[record + 2];
        const auto flags = mem.bytes[record + 3];
        const auto source = std::uint16_t(shell_image + 12 + offset);
        const auto target = std::uint16_t(relocated_code + offset);
        if (width == 2) {
            require(mem.word(target) ==
                        std::uint16_t(mem.word(source) + code_base),
                    "XL word relocation produced the wrong target");
        } else {
            const auto addend = std::uint8_t(flags & 1 ? code_base >> 8
                                                       : code_base);
            require(mem.bytes[target] ==
                        std::uint8_t(mem.bytes[source] + addend),
                    "XL byte relocation produced the wrong target");
        }
    }

    const auto put_string = [&](std::uint16_t address, const std::string& value) {
        for (std::size_t i = 0; i <= value.size(); ++i)
            mem.bytes[std::uint16_t(address + i)] = value.c_str()[i];
    };
    const auto retire_process = [&](std::uint16_t process) {
        const auto thread = mem.word(process + 13);
        call_kernel(sym("_list_remove"), sym("_thread_first_running"),
                    thread, "retire: unlink thread");
        call_kernel(sym("_list_insert"), sym("_thread_first_terminated"),
                    thread, "retire: terminate thread");
        mem.bytes[thread + 19] = 4;
        mem.word(sym("_thread_current"), 0);
        call_kernel(sym("__thread_cleanup_terminated"), 0, 0,
                    "retire: scheduler cleanup");
    };
    // Load the actual shell through the real ROM/POSIX/XL path, then let
    // it load, initialize and call the separately packaged library.
    files.enabled = true;
    put_string(0xe100, "shell.sys");
    call_kernel(sym("_enter_critical_section"), 0, 0, "outer disk critical section");
    const auto disk_fd = call_kernel(sym("_open"), 0xe100, 0, "nested disk open");
    require(disk_fd != 0xffff && !cpu.snapshot().iff1 &&
                mem.bytes[sym("__interrupt_refcount")] == 0x81,
            "disk open released the caller's critical section: fd=" +
                std::to_string(disk_fd) + " iff=" +
                std::to_string(cpu.snapshot().iff1) + " depth=" +
                std::to_string(mem.bytes[sym("__interrupt_refcount")]));
    call_kernel(sym("_close"), disk_fd, 0, "nested disk close");
    put_string(0xe120, "missing.sys");
    require(call_kernel(sym("_open"), 0xe120, 0, "nested disk error") == 0xffff &&
                !cpu.snapshot().iff1 &&
                mem.bytes[sym("__interrupt_refcount")] == 0x81,
            "disk error released the caller's critical section");
    call_kernel(sym("_leave_critical_section"), 0, 0, "leave disk critical section");
    require(cpu.snapshot().iff1 && !mem.bytes[sym("__interrupt_refcount")],
            "disk critical section did not restore preemption");
    const auto loaded_shell = call_kernel(sym("_process_load"), 0xe100, 0,
                                          "load shell from mock esxDOS");
    require(loaded_shell != 0,
            "shell load failed: " +
                std::to_string(mem.bytes[sym("_process_last_error")]));
    // XL v2 keeps the relocation table after the code, so the retained
    // shell block is exactly its code: the XPRG/XL metadata prefix and the
    // consumed relocation table were split off and freed.
    bool shell_block_is_code_only = false;
    for (auto block = sym("__heap"), n = std::uint16_t(0); block && n < 256;
         block = mem.word(block), ++n) {
        if ((mem.bytes[block + 4] & 1) && mem.word(block + 2) == loaded_shell &&
            mem.word(block + 5) == code_size)
            shell_block_is_code_only = true;
    }
    require(shell_block_is_code_only,
            "shell image block still carries metadata or its relocation table");
    const auto shell_thread = mem.word(loaded_shell + 13);
    mem.word(sym("_thread_current"), shell_thread);
    unsigned staged_registrations = 0;
    files.observe = [&] {
        const auto state = cpu.snapshot();
        const auto library = mem.word(shell_thread + 2);
        if (!library || state.ix < 0x4000 || state.ix > 0xffaf) return;
        const auto base = mem.word(state.ix + 68);
        if (state.pc != base + library_symbols.at("_registered")) return;
        ++staged_registrations;
        require(mem.word(shell_thread + 22) == loaded_shell,
                "initializer changed the client's thread membership");
        for (auto service = mem.word(sym("__svc_first")); service;
             service = mem.word(service)) {
            require(mem.word(service + 2) != library,
                    "initializer prematurely published its service");
        }
        const auto staged = mem.word(sym("__library_private_services"));
        require(staged && mem.word(staged + 2) == library,
                "initializer registration was not staged under its library");
    };
    auto shell_state = cpu.snapshot();
    shell_state.halted = false;
    shell_state.pc = shell_thread + 6;
    shell_state.sp = mem.word(shell_thread + 4) + 22;
    cpu.restore(shell_state);
    run_until([&] {
        const auto pc = cpu.pc();
        return mem.bytes[pc] == 0x18 && mem.bytes[std::uint16_t(pc + 1)] == 0xfe;
    }, 5000000, "relocated shell GPX drawing");
    files.observe = {};
    require(staged_registrations == 1,
            "shell did not execute the self-registering initializer once");
    bool shell_drew_pixels = false;
    for (std::uint16_t address = 0x4000; address != 0x5800; ++address)
        shell_drew_pixels = shell_drew_pixels || mem.bytes[address] != 0;
    require(shell_drew_pixels,
            "relocated shell reached its loop without drawing text");
    put_string(0xe100, "shelllib");
    const auto shell_library = call_kernel(sym("__svc_query"), 0xe100, 0,
                                           "shell's registered library");
    require(shell_library != 0, "shell did not register shelllib: error " +
                std::to_string(mem.bytes[sym("_process_last_error")]));
    require(call_kernel(mem.word(shell_library + 4), 0, 0,
                        "shell library init count") == 1,
            "library initializer did not run exactly once");
    require(call_kernel(mem.word(shell_library + 6), 0, 0,
                        "shell library call count") == 1,
            "shell did not call the library's relocated function");
    require(mem.word(shell_thread + 2) == 0 &&
                mem.word(shell_thread + 22) == loaded_shell,
            "library initialization did not restore caller ownership");
    require(files.handles.empty(), "shell/library load leaked descriptors");
    retire_process(loaded_shell);
    require(mem.word(sym("_process_first")) == 0 &&
                mem.word(sym("__library_refs")) == 0,
            "shell exit did not release its library");
    require(call_kernel(sym("__svc_query"), 0xe100, 0,
                        "unloaded shell library") == 0,
            "last-client cleanup left a published service");
    files.enabled = false;

    // Physical-drive prefixes belong to YOS, not the native firmware path.
    for (const auto& path : {std::string("A:/"), std::string("b:/TOOLS"),
                             std::string("ALTO.SYS"), std::string("/"),
                             std::string("C:/")}) {
        for (std::size_t i = 0; i <= path.size(); ++i)
            mem.bytes[0x8300 + i] = path.c_str()[i];
        const bool qualified = path.size() > 1 && path[1] == ':' && path[0] != 'C';
        call_kernel(sym("__zx_esx_path_drive"), 0x8300, 0x1234,
                    "drive pathname translation", 0x2a, {}, 0x4567);
        const auto result = cpu.snapshot();
        require(result.hl == 0x8300 + (qualified ? 2 : 0) &&
                    (result.af >> 8) == (qualified ? (path[0] == 'A' ? 0x40 : 0x48) : 0x2a) &&
                    result.de == 0x1234 && result.bc == 0x4567,
                "drive prefix changed the pathname or firmware argument registers");
    }

    constexpr std::uint16_t native_dirent = 0x8300;
    constexpr std::uint16_t public_dirent = 0x8340;
    const std::string regular_name = "FILE.TXT";
    for (std::size_t i = 0; i <= regular_name.size(); ++i)
        mem.bytes[native_dirent + 1 + i] = regular_name.c_str()[i];
    const auto native_tail = native_dirent + regular_name.size() + 1;
    mem.bytes[native_dirent] = 0x20;
    mem.bytes[native_tail + 5] = 0x78;
    mem.bytes[native_tail + 6] = 0x56;
    mem.bytes[native_tail + 7] = 0x34;
    mem.bytes[native_tail + 8] = 0x12;
    call_kernel(sym("__directory_convert"), native_dirent, public_dirent,
                "directory entry conversion");
    require(mem.bytes[public_dirent + 8] == 8 &&
                mem.bytes[public_dirent + 9] == 0x20,
            "regular directory entry type or attributes are wrong");
    require(mem.word(public_dirent + 4) == 0x5678 &&
                mem.word(public_dirent + 6) == 0x1234,
            "regular directory entry size is wrong");
    for (std::size_t i = 0; i <= regular_name.size(); ++i) {
        require(mem.bytes[public_dirent + 10 + i] == regular_name.c_str()[i],
                "regular directory entry name is wrong");
    }
    mem.bytes[native_dirent] = 0x10;
    call_kernel(sym("__directory_convert"), native_dirent, public_dirent,
                "directory type conversion");
    require(mem.bytes[public_dirent + 8] == 4,
            "directory entry was not reported as DT_DIR");

    const auto public_block =
        call_kernel(mem.word(table + 2), 23, 0, "public allocation");
    require(public_block != 0, "public malloc wrapper failed");
    call_kernel(mem.word(table + 4), public_block, 0, "public release");

    // shrink_memory releases the tail of a live block in place: the block
    // keeps its address, its owner and the requested size, and the released
    // bytes become a free block that later allocations can reuse.
    const auto block_size = [&](std::uint16_t payload) {
        return mem.word(std::uint16_t(payload - 2));
    };
    const auto shrink_block =
        call_kernel(mem.word(table + 2), 200, 0, "shrinkable allocation");
    require(shrink_block != 0 && block_size(shrink_block) == 200,
            "shrinkable allocation is not 200 bytes");
    const auto shrink_owner = mem.word(std::uint16_t(shrink_block - 5));
    require(call_kernel(mem.word(table + 96), shrink_block, 100,
                        "public shrink") == shrink_block &&
                block_size(shrink_block) == 100 &&
                mem.word(std::uint16_t(shrink_block - 5)) == shrink_owner,
            "shrink_memory did not trim the block in place");
    const auto released = std::uint16_t(shrink_block + 100);
    require(mem.word(std::uint16_t(shrink_block - 7)) == released &&
                (mem.bytes[released + 4] & 1) == 0,
            "shrink_memory did not turn the tail into a free block");
    const auto reused =
        call_kernel(mem.word(table + 2), 60, 0, "allocation from the tail");
    require(reused == std::uint16_t(released + 7),
            "released tail was not reused by the next allocation");
    call_kernel(mem.word(table + 4), reused, 0, "release the reused tail");
    require(call_kernel(mem.word(table + 96), shrink_block, 95,
                        "shrink below a splittable remainder") ==
                    shrink_block &&
                block_size(shrink_block) == 100,
            "shrink_memory split off a remainder too small for a block");
    require(call_kernel(mem.word(table + 96), shrink_block, 300,
                        "shrink to a larger size") == shrink_block &&
                block_size(shrink_block) == 100,
            "shrink_memory changed a block for a larger size");
    call_kernel(mem.word(table + 4), shrink_block, 0, "release shrunk block");
    require(call_kernel(mem.word(table + 96), shrink_block, 10,
                        "shrink a freed block") == 0,
            "shrink_memory accepted a freed block");
    const auto public_timer =
        call_kernel(mem.word(table + 12), 0x4200, 3,
                    "public timer installation");
    require(public_timer != 0, "public timer wrapper failed");
    call_kernel(mem.word(table + 14), public_timer, 0,
                "public timer removal");
    require(call_kernel(mem.word(table + 6), 0, 0, "public clock") == 0,
            "clock advanced before an interrupt");

    constexpr std::uint16_t mouse_state = 0x8210;
    io.mouse_x = 17;
    io.mouse_y = 31;
    call_kernel(mem.word(table + 46), 23, 0, "mouse calibration", 42);
    call_kernel(mem.word(table + 48), mouse_state, 0, "mouse snapshot");
    require(mem.bytes[mouse_state] == 42 && mem.bytes[mouse_state + 1] == 23 &&
                mem.bytes[mouse_state + 2] == 0 &&
                mem.bytes[mouse_state + 3] == 0,
            "stationary calibrated mouse state is wrong");
    io.mouse_x = 20;
    io.mouse_y = 29;
    io.mouse_buttons = 0xfe;
    call_kernel(mem.word(table + 48), mouse_state, 0,
                "mouse snapshot before timer scan");
    require(mem.bytes[mouse_state] == 42 && mem.bytes[mouse_state + 1] == 23 &&
                mem.bytes[mouse_state + 2] == 0 &&
                mem.bytes[mouse_state + 3] == 0,
            "read_mouse polled hardware instead of reading timer state");
    call_kernel(sym("__tmr_chain"), 0, 0, "moving mouse timer-chain scan");
    call_kernel(mem.word(table + 48), mouse_state, 0, "moving mouse snapshot");
    require(mem.bytes[mouse_state] == 45 && mem.bytes[mouse_state + 1] == 25 &&
                mem.bytes[mouse_state + 2] == 1 &&
                mem.bytes[mouse_state + 3] == 1,
            "moving mouse state is wrong: " +
                std::to_string(mem.bytes[mouse_state]) + "," +
                std::to_string(mem.bytes[mouse_state + 1]) + "," +
                std::to_string(mem.bytes[mouse_state + 2]) + "," +
                std::to_string(mem.bytes[mouse_state + 3]));
    call_kernel(mem.word(table + 48), mouse_state, 0,
                "stationary mouse resnapshot");
    require(mem.bytes[mouse_state] == 45 && mem.bytes[mouse_state + 1] == 25 &&
                mem.bytes[mouse_state + 2] == 1 &&
                mem.bytes[mouse_state + 3] == 0,
            "stationary mouse retained stale change flags");
    io.mouse_buttons = 0xff;
    call_kernel(sym("__mouse_scan"), 0, 0, "mouse release timer scan");
    io.mouse_buttons = 0xfe;
    call_kernel(sym("__mouse_scan"), 0, 0, "mouse repress timer scan");
    call_kernel(mem.word(table + 48), mouse_state, 0,
                "accumulated mouse transition snapshot");
    require(mem.bytes[mouse_state + 2] == 1 &&
                mem.bytes[mouse_state + 3] == 1,
            "mouse timer lost an unread button transition");

    constexpr std::uint16_t gpx_name = 0x8240;
    mem.bytes[gpx_name] = 'g';
    mem.bytes[gpx_name + 1] = 'p';
    mem.bytes[gpx_name + 2] = 'x';
    mem.bytes[gpx_name + 3] = 0;
    const auto gpx_table =
        call_kernel(0x0018, gpx_name, 0, "RST 18 gpx service query");
    require(gpx_table == sym("__gpx_service"),
            "gpx service did not return its ROM table");
    constexpr std::array<const char*, 24> gpx_api = {
        "_gpx_create", "_gpx_destroy", "_gpx_set_page",
        "_gpx_width", "_gpx_height", "_gpx_clrscr",
        "_gpx_set_text_background", "_gpx_draw_pixel",
        "_gpx_draw_line", "_gpx_draw_bmp", "_gpx_show_sprite",
        "_gpx_hide_sprite", "_gpx_draw_rectangle",
        "_gpx_fill_rectangle", "_gpx_measure_text", "_gpx_draw_text",
        "_gpx_get_system_font", "_gpx_get_tiny_font",
        "_gpx_get_stock_bmp", "_gpx_draw_circle", "_gpx_fill_circle",
        "_gpx_draw_polygon", "_gpx_fill_polygon", "_gpx_draw_box"
    };
    for (std::size_t slot = 0; slot < gpx_api.size(); ++slot) {
        require(mem.word(gpx_table + 2 * slot) == sym(gpx_api[slot]),
                "GPX API slot " + std::to_string(slot) +
                    " does not match " + gpx_api[slot]);
    }

    const auto gpx_context =
        call_kernel(mem.word(gpx_table), 0, 0, "gpx creation");
    require(gpx_context >= sym("__heap"),
            "gpx creation did not allocate a private context");
    require(mem.word(gpx_context) == 256 &&
                mem.word(gpx_context + 2) == 192 &&
                mem.bytes[gpx_context + 4] == 1 &&
                mem.bytes[gpx_context + 5] == 0,
            "gpx context has the wrong Spectrum geometry");
    call_kernel(mem.word(gpx_table + 12), gpx_context, 0,
                "gpx transparent text mode", 0, {1});
    require(mem.bytes[gpx_context + 5] == 1,
            "gpx text-background mode was not stored");
    const auto second_context =
        call_kernel(sym("_gpx_create"), 0, 0, "independent gpx context");
    require(second_context && second_context != gpx_context &&
                mem.bytes[second_context + 5] == 0 &&
                mem.bytes[gpx_context + 5] == 1,
            "GPX contexts share or reset drawing state");
    call_kernel(sym("_gpx_destroy"), second_context, 0, "destroy second context");
    require(call_kernel(mem.word(gpx_table + 6), 0, 0,
                        "gpx width") == 256,
            "gpx width is wrong");
    require(call_kernel(mem.word(gpx_table + 8), 0, 0,
                        "gpx height") == 192,
            "gpx height is wrong");
    require(call_kernel(mem.word(gpx_table + 32), 0, 0,
                        "gpx system font") != 0 &&
                call_kernel(mem.word(gpx_table + 34), 0, 0,
                            "gpx tiny font") != 0 &&
                call_kernel(mem.word(gpx_table + 36), 0, 0,
                            "gpx stock bitmap", 0) != 0,
            "gpx built-in assets are unavailable");
    require(call_kernel(mem.word(gpx_table + 36), 0, 0,
                        "gpx resize cursor", 5) != 0,
            "gpx resize cursor is unavailable");
    mem.bytes[0x4000] = 0xff;
    mem.bytes[0x5aff] = 0xff;
    call_kernel(mem.word(gpx_table + 10), 0, 0, "gpx screen clear");
    require(mem.bytes[0x4000] == 0 && mem.bytes[0x5aff] == 0x38,
            "gpx clear did not cover the Spectrum framebuffer");
    call_kernel(mem.word(gpx_table + 14), gpx_context, 8,
                "gpx pixel drawing", 0, {10, 0, 1, 0, 0, 0});
    require(mem.bytes[0x4221] == 0x80,
            "gpx pixel drawing wrote the wrong Spectrum byte");
    constexpr std::uint16_t box = 0x8250;
    mem.word(box, 8);
    mem.word(box + 2, 10);
    mem.word(box + 4, 15);
    mem.word(box + 6, 10);
    call_kernel(mem.word(gpx_table + 46), gpx_context, box,
                "gpx selected-edge box", 0, {2, 1, 0, 0xff, 0, 0});
    require(mem.bytes[0x4221] == 0xff,
            "gpx selected-edge box drew the wrong Spectrum byte");

    call_kernel(mem.word(table + 44), 0, 0, "empty keyboard queue");
    require(kernel_hl == 0,
            "keyboard queue was not empty after boot");
    io.keyboard_address = 0xf7fe;
    io.keyboard_value = 0xfe;
    call_kernel(sym("__kbd_scan"), 0, 0, "keyboard press scan");
    call_kernel(mem.word(table + 44), 0, 0, "keyboard press read");
    const auto key_press = kernel_hl & 0xff;
    require(key_press == 0x45,
            "keyboard press event is wrong: " + std::to_string(key_press));
    io.keyboard_value = 0xff;
    call_kernel(sym("__kbd_scan"), 0, 0, "keyboard release scan");
    call_kernel(mem.word(table + 44), 0, 0, "keyboard release read");
    const auto key_release = kernel_hl & 0xff;
    require(key_release == 5,
            "keyboard release event is wrong: " + std::to_string(key_release));

    // Multiple transitions in one row must retain the bit countdown after
    // queueing each event. Check both edges in every row (modifiers excluded).
    std::uint8_t row_select = 0xf7;
    for (unsigned row = 0; row < 8; ++row) {
        io.keyboard_address = (std::uint16_t(row_select) << 8) | 0xfe;
        const unsigned mask = row == 4 ? 0x1d : row == 5 ? 0x1e : 0x1f;
        for (const bool pressed : {true, false}) {
            io.keyboard_value = pressed ? std::uint8_t(~mask) : 0xff;
            call_kernel(sym("__kbd_scan"), 0, 0, "keyboard chord scan");
            for (unsigned bit = 0; bit < 5; ++bit) {
                if (!(mask & (1u << bit))) continue;
                call_kernel(mem.word(table + 44), 0, 0, "keyboard chord read");
                const unsigned expected = row * 5 + 4 - bit +
                                          (pressed ? 0x40 : 0) + 1;
                require((kernel_hl & 0xff) == expected,
                        "keyboard chord row " + std::to_string(row) +
                        " bit " + std::to_string(bit) + " produced " +
                        std::to_string(kernel_hl & 0xff));
            }
            call_kernel(mem.word(table + 44), 0, 0, "keyboard chord drained");
            require((kernel_hl & 0xff) == 0, "keyboard chord queued extra keys");
        }
        row_select = std::uint8_t((row_select << 1) | (row_select >> 7));
    }

    constexpr std::uint16_t status_buffer = 0x8200;
    constexpr std::array<std::size_t, 6> invalid_fd_slots = {
        27, 28, 29, 31, 30, 39};
    for (const auto slot : invalid_fd_slots) {
        require(call_kernel(mem.word(table + 2 * slot), 0, status_buffer,
                            "filesystem rejects unopened descriptor") == 0xffff,
                "filesystem accepted unopened descriptor zero");
        require(mem.word(sym("__errno_value")) == 9,
                "unopened descriptor did not set EBADF");
    }
    constexpr std::array<std::size_t, 7> invalid_path_slots = {
        26, 32, 33, 34, 36, 37, 38};
    for (const auto slot : invalid_path_slots) {
        require(call_kernel(mem.word(table + 2 * slot), 0, status_buffer,
                            "filesystem rejects null path") == 0xffff,
                "filesystem accepted a null path");
        require(mem.word(sym("__errno_value")) == 14,
                "null path did not set EFAULT");
    }
    require(call_kernel(mem.word(table + 80), 0, 0,
                        "opendir rejects null path") == 0,
            "opendir accepted a null path");
    require(mem.word(sym("__errno_value")) == 14,
            "null directory path did not set EFAULT");
    require(call_kernel(mem.word(table + 82), 0, 0,
                        "readdir rejects null directory") == 0,
            "readdir accepted a null directory");
    require(mem.word(sym("__errno_value")) == 9,
            "null directory did not set EBADF");
    require(call_kernel(mem.word(table + 86), 0, 0,
                        "closedir rejects null directory") == 0xffff,
            "closedir accepted a null directory");
    require(call_kernel(mem.word(table + 88), 0, 1,
                        "disk enumeration rejects null buffer") == 0xffff,
            "disk enumeration accepted a null output buffer");
    require(mem.word(sym("__errno_value")) == 14,
            "null disk output buffer did not set EFAULT");
    require(call_kernel(mem.word(table + 88), 0, 0,
                        "empty disk enumeration") == 0,
            "zero-capacity disk enumeration failed");
    require(call_kernel(mem.word(table + 88), status_buffer, 0x0100,
                        "disk enumeration rejects oversized capacity") ==
                0xffff,
            "disk enumeration accepted capacity above 255");
    require(mem.word(sym("__errno_value")) == 22,
            "oversized disk capacity did not set EINVAL");
    require(call_kernel(mem.word(table + 88), 0xfffc, 1,
                        "disk enumeration rejects wrapped buffer") == 0xffff,
            "disk enumeration accepted a wrapping output buffer");
    require(mem.word(sym("__errno_value")) == 14,
            "wrapped disk output buffer did not set EFAULT");

    const auto user_heap = sym("__heap");
    test_thread_safety(mem, cpu, call_kernel, sym, files, gpx_context);
    call_kernel(sym("_gpx_destroy"), gpx_context, 0, "destroy GPX context");
    require(mem.word(user_heap) == 0, "new user heap is not a single block");
    const auto expected_heap_size = std::uint16_t(0xffff - user_heap - 7);
    require(mem.word(user_heap + 5) == expected_heap_size,
            "user heap size is wrong: got " +
                std::to_string(mem.word(user_heap + 5)) + " expected " +
                std::to_string(expected_heap_size));

    constexpr std::uint16_t name = 0x8000;
    constexpr std::uint16_t entry = 0x4000;
    constexpr std::uint16_t return_pc = 0x4100;
    constexpr std::uint16_t call_sp = 0xf000;
    mem.bytes[name] = 't';
    mem.bytes[name + 1] = '0';
    mem.bytes[name + 2] = 0;
    mem.bytes[entry] = 0x00;
    mem.bytes[return_pc] = 0x76;
    mem.word(call_sp - 2, 128);
    mem.word(call_sp - 4, return_pc);

    state = cpu.snapshot();
    state.halted = false;
    state.pc = sym("_process_start");
    state.sp = call_sp - 4;
    state.hl = name;
    state.de = entry;
    state.ix = 0xa55a;
    state.iy = 0x5aa5;
    cpu.restore(state);
    run_until([&] { return cpu.pc() == return_pc; }, 500000,
              "first process_start");
    state = cpu.snapshot();
    const auto process = state.de;
    require(process != 0, "process_start failed");
    require(state.sp == call_sp, "process_start stack cleanup is wrong");
    require(state.ix == 0xa55a && state.iy == 0x5aa5,
            "process_start changed a preserved index register");
    require(mem.word(sym("_process_first")) == process,
            "process was not linked");
    require(mem.bytes[process + 5] == 't' &&
                mem.bytes[process + 6] == '0' &&
                mem.bytes[process + 7] == 0,
            "process name was not copied");

    const auto thread = mem.word(process + 13);
    require(thread != 0, "main thread was not created");
    require(mem.word(thread + 22) == process, "thread owner is wrong");
    require(mem.bytes[thread + 19] == 1, "thread is not runnable");
    require(mem.word(sym("_thread_first_running")) == thread,
            "thread was not linked into the runnable queue");
    require(mem.word(sym("_thread_first_suspended")) == 0,
            "thread remained suspended");
    require(mem.bytes[thread + 6] == 0xcd &&
                mem.word(thread + 7) == entry &&
                mem.bytes[thread + 9] == 0x21 &&
                mem.word(thread + 10) == thread &&
                mem.bytes[thread + 12] == 0xc3 &&
                mem.word(thread + 13) == sym("_thread_exit"),
            "thread startup program is malformed");
    const auto thread_sp = mem.word(thread + 4);
    require(mem.word(thread_sp + 20) == thread + 6,
            "thread initial return address is wrong");

    constexpr std::uint16_t name2 = 0x8020;
    constexpr std::uint16_t entry2 = 0x4010;
    mem.bytes[name2] = 't';
    mem.bytes[name2 + 1] = '1';
    mem.bytes[name2 + 2] = 0;
    mem.word(call_sp - 2, 128);
    mem.word(call_sp - 4, return_pc);
    state = cpu.snapshot();
    state.halted = false;
    state.pc = sym("_process_start");
    state.sp = call_sp - 4;
    state.hl = name2;
    state.de = entry2;
    state.ix = 0xa55a;
    state.iy = 0x5aa5;
    cpu.restore(state);
    run_until([&] { return cpu.pc() == return_pc; }, 500000,
              "second process_start");
    state = cpu.snapshot();
    const auto process2 = state.de;
    const auto thread2 = mem.word(process2 + 13);
    require(process2 != 0 && thread2 != 0, "second process failed");
    require(mem.word(sym("_thread_first_running")) == thread2 &&
                mem.word(thread2) == thread,
            "two-thread runnable queue is malformed");

    state = cpu.snapshot();
    state.halted = false;
    state.pc = 0x4200;
    state.sp = sym("__sys_stack");
    state.im = 2;
    state.i = 0x5e;
    state.iff1 = true;
    state.iff2 = true;
    cpu.restore(state);
    require(cpu.interrupt(0xff), "maskable interrupt was rejected");
    run_until([&] { return cpu.pc() == entry2; }, 500000,
              "first interrupt");
    require(mem.word(sym("_thread_current")) == thread2,
            "interrupt did not select the first runnable thread");
    require(!mem.word(sym("__errno_value")) &&
                !mem.bytes[sym("_process_last_error")],
            "new thread inherited another thread's error values");
    mem.word(sym("__errno_value"), 0x1234);
    mem.bytes[sym("_process_last_error")] = 8;

    state = cpu.snapshot();
    const auto thread2_sp = state.sp;
    state.pc = entry2 + 1;
    state.af = 0x1234;
    state.bc = 0x2345;
    state.de = 0x3456;
    state.hl = 0x4567;
    state.ix = 0x5678;
    state.iy = 0x6789;
    state.af2 = 0x789a;
    state.bc2 = 0x89ab;
    state.de2 = 0x9abc;
    state.hl2 = 0xabcd;
    state.iff1 = true;
    state.iff2 = true;
    cpu.restore(state);
    require(cpu.interrupt(0xff), "second interrupt was rejected");
    run_until([&] { return cpu.pc() == entry; }, 500000,
              "second interrupt");
    require(mem.word(sym("_thread_current")) == thread,
            "round robin did not select the second runnable thread");
    require(!mem.word(sym("__errno_value")) &&
                !mem.bytes[sym("_process_last_error")] &&
                mem.word(thread2 + 20) == 0x1234 &&
                mem.bytes[thread2 + 15] == 8,
            "thread switch did not save and isolate syscall errors");
    mem.word(sym("__errno_value"), 0x4567);
    mem.bytes[sym("_process_last_error")] = 9;
    // EI immediately precedes RETI in the scheduler, so a newly requested
    // interrupt is accepted after one instruction from the resumed thread.
    require(mem.word(mem.word(thread2 + 4) + 20) == entry2 + 2,
            "saved thread return PC is wrong: got " +
                std::to_string(mem.word(mem.word(thread2 + 4) + 20)));

    state = cpu.snapshot();
    state.pc = entry + 1;
    state.af = 0xcdef;
    state.bc = 0xdef0;
    state.de = 0xef01;
    state.hl = 0xf012;
    state.ix = 0x0123;
    state.iy = 0x1357;
    state.af2 = 0x2468;
    state.bc2 = 0x369c;
    state.de2 = 0x48ad;
    state.hl2 = 0x5abe;
    state.iff1 = true;
    state.iff2 = true;
    cpu.restore(state);
    require(cpu.interrupt(0xff), "third interrupt was rejected");
    run_until([&] { return cpu.pc() == entry2 + 2 || cpu.halted(); }, 500000,
              "third interrupt");
    state = cpu.snapshot();
    require(cpu.pc() == entry2 + 2,
            "third interrupt halted at PC " + std::to_string(cpu.pc()) +
                ", current " +
                std::to_string(mem.word(sym("_thread_current"))) +
                ", expected thread " + std::to_string(thread2));
    require(mem.word(sym("_thread_current")) == thread2,
            "round robin did not wrap to the queue head");
    require(mem.word(sym("__errno_value")) == 0x1234 &&
                mem.bytes[sym("_process_last_error")] == 8 &&
                mem.word(thread + 20) == 0x4567 && mem.bytes[thread + 15] == 9,
            "resumed thread did not recover its syscall errors");
    require(state.sp == thread2_sp && state.af == 0x1234 &&
                state.bc == 0x2345 && state.de == 0x3456 &&
                state.hl == 0x4567 && state.ix == 0x5678 &&
                state.iy == 0x6789 && state.af2 == 0x789a &&
                state.bc2 == 0x89ab && state.de2 == 0x9abc &&
                state.hl2 == 0xabcd,
            "saved thread context was not restored exactly");

    // Exercise the waiting-event path without adding a public wait API that
    // the preserved C kernel does not implement.
    constexpr std::uint16_t waits = 0x8100;
    constexpr std::uint16_t event = 0x8120;
    mem.word(sym("_thread_first_running"), thread2);
    mem.word(thread2, 0);
    mem.word(sym("_thread_first_waiting"), thread);
    mem.word(thread, 0);
    mem.word(thread + 16, waits);
    mem.bytes[thread + 18] = 1;
    mem.bytes[thread + 19] = 2;
    mem.word(waits, event);
    mem.bytes[event + 4] = 1;
    mem.word(call_sp - 2, return_pc);
    state = cpu.snapshot();
    state.halted = false;
    state.pc = sym("__thread_select_next");
    state.sp = call_sp - 2;
    state.ix = 0xa55a;
    state.iy = 0x5aa5;
    cpu.restore(state);
    run_until([&] { return cpu.pc() == return_pc; }, 500000,
              "waiting-thread selection");
    state = cpu.snapshot();
    require(state.de == thread && state.sp == call_sp,
            "woken thread was not selected: result " +
                std::to_string(state.de) + ", expected " +
                std::to_string(thread) + ", waiting " +
                std::to_string(mem.word(sym("_thread_first_waiting"))) +
                ", running " +
                std::to_string(mem.word(sym("_thread_first_running"))) +
                ", count " + std::to_string(mem.bytes[thread + 18]) +
                ", state " + std::to_string(mem.bytes[thread + 19]) +
                ", waits " + std::to_string(mem.word(thread + 16)) +
                ", event " + std::to_string(mem.word(waits)) +
                ", event state " + std::to_string(mem.bytes[event + 4]));
    require(mem.word(sym("_thread_first_waiting")) == 0 &&
                mem.word(sym("_thread_first_running")) == thread &&
                mem.word(thread) == thread2 && mem.bytes[thread + 19] == 1,
            "signaled thread was not moved to the runnable queue");

    // Simulate the state seen after a terminated thread has switched away.
    // The next selection must release its stack, descriptor and empty process.
    mem.word(sym("_thread_first_running"), thread2);
    mem.word(thread2, 0);
    mem.word(sym("_thread_first_terminated"), thread);
    mem.word(thread, 0);
    mem.bytes[thread + 19] = 4;
    mem.word(sym("_thread_current"), thread2);
    mem.word(call_sp - 2, return_pc);
    state = cpu.snapshot();
    state.halted = false;
    state.pc = sym("__thread_select_next");
    state.sp = call_sp - 2;
    state.ix = 0xa55a;
    state.iy = 0x5aa5;
    cpu.restore(state);
    run_until([&] { return cpu.pc() == return_pc; }, 500000,
              "terminated-thread cleanup");
    state = cpu.snapshot();
    require(state.de == thread2, "cleanup lost the runnable thread");
    require(mem.word(sym("_thread_first_terminated")) == 0,
            "terminated thread was not removed");
    require(mem.word(sym("_process_first")) == process2 &&
                mem.word(process2) == 0,
            "empty process was not reaped");
    require(state.sp == call_sp && state.ix == 0xa55a &&
                state.iy == 0x5aa5,
            "scheduler cleanup violated its ABI");
    test_libraries(mem, cpu, call_kernel, sym, files, library_image,
                   library_symbols, put_string);
    require(mem.rom_writes == 0, "kernel attempted to write into ROM");

    std::cout << "PASS: boot, XPRG CRC and relocation, heaps, syscalls, "
                 "nested interrupt-state preservation, protected shared state, "
                 "per-thread errors and concurrent library loading, "
                 "GPX service and drawing, directory records, two-process round robin, "
                 "event wakeup, terminated-process cleanup, shell library "
                 "self-registration, shared/private lifetime and rollback\n";
}
