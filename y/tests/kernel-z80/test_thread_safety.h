// Shared-state and framebuffer critical-section regressions.
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
#ifndef YOS_TEST_THREAD_SAFETY_H
#define YOS_TEST_THREAD_SAFETY_H

#include <stdexcept>

template<class Memory, class Cpu, class Call, class Symbol, class Files>
void test_thread_safety(Memory& mem, Cpu& cpu, Call call, Symbol sym,
                        Files& files, std::uint16_t context)
{
    const auto check = [](bool ok, const char* message) {
        if (!ok) throw std::runtime_error(message);
    };
    unsigned writes = 0;
    bool race = false;
    const auto fd_table = sym("__zx_esx_files");
    const auto mouse = sym("__mouse_cursor");
    const auto events = sym("__evt_first"), timers = sym("__tmr_first");
    const auto heap = sym("__sys_heap"), heap_end = sym("__heap");
    mem.observe = [&](std::uint16_t address, bool write) {
        if (write && ((address >= fd_table && address < fd_table + 32) ||
                (address >= mouse && address < mouse + 6) ||
                (address >= events && address < events + 2) ||
                (address >= timers && address < timers + 2) ||
                (address >= heap && address < heap_end))) {
            ++writes;
            race |= cpu.snapshot().iff1;
        }
    };
    const auto event = call(sym("_evt_create"), 0, 0, "protected event create");
    check(event != 0, "event allocation failed");
    call(sym("_evt_set"), event, 0, "protected event set", 0, {1});
    check(mem.bytes[event + 4] == 1, "event was not signalled");
    auto state = cpu.snapshot();
    state.iff1 = state.iff2 = false;
    cpu.restore(state);
    call(sym("_evt_set"), event, 0, "interrupt-context event set", 0, {0});
    check(!cpu.snapshot().iff1 && !mem.bytes[event + 4],
          "event callback enabled IRQ inside the scheduler");
    state = cpu.snapshot();
    state.iff1 = state.iff2 = true;
    cpu.restore(state);
    call(sym("_evt_destroy"), event, 0, "protected event destroy");
    const auto timer = call(sym("__yos_install_timer"), 0xe600, 2,
                            "protected timer create");
    check(timer != 0, "timer allocation failed");
    call(sym("_tmr_uninstall"), timer, 0, "protected timer destroy");
    call(sym("_mouse_calibrate"), 20, 0, "protected mouse calibration", 30);
    call(sym("_mouse_read"), 0xe600, 0, "protected mouse read");
    files.enabled = true;
    const std::string path = "shell.sys";
    std::copy(path.c_str(), path.c_str() + path.size() + 1,
              mem.bytes.begin() + 0xe600);
    bool checked_slot = false;
    files.observe = [&] {
        if (cpu.pc() == sym("__zx_esx_free_fd")) {
            checked_slot = true;
            race |= cpu.snapshot().iff1;
        }
    };
    const auto first = call(sym("_open"), 0xe600, 0, "protected fd reservation");
    const auto second = call(sym("_open"), 0xe600, 0, "second protected fd");
    check(first != 0xffff && second != 0xffff && first != second,
          "open reused an active descriptor");
    call(sym("_close"), first, 0, "protected fd close");
    call(sym("_close"), second, 0, "second protected fd close");
    files.files["append"] = {1, 2};
    const std::string append_path = "append";
    std::copy(append_path.c_str(), append_path.c_str() + append_path.size() + 1,
              mem.bytes.begin() + 0xe600);
    const auto append = call(sym("_open"), 0xe600, 0x0402, "append open");
    check(append != 0xffff, "append file open failed");
    bool in_append = false, saw_write = false;
    files.observe = [&] {
        if (cpu.pc() == files.status_gate) in_append = true;
        if (in_append) race |= cpu.snapshot().iff1;
        if (cpu.pc() == files.write_gate) {
            saw_write = true;
            in_append = false;
        }
    };
    mem.bytes[0xe620] = 3;
    check(call(sym("_write"), append, 0xe620, "atomic append", 0,
               {1, 0}, 0, true) == 1 && saw_write &&
              files.files.at("append") == std::vector<std::uint8_t>({1, 2, 3}),
          "append seek/write was not one protected transaction");
    files.observe = {};
    check(call(sym("_lseek"), append, 0, "protected seek", 0,
               {0, 0, 0, 0, 0, 0}, 0, true) == 0,
          "seek failed after append");
    check(call(sym("_read"), append, 0xe620, "protected read", 0,
               {3, 0}, 0, true) == 3 && mem.bytes[0xe622] == 3,
          "read failed after append");
    check(call(sym("_fstat"), append, 0xe640, "protected fstat") == 0 &&
              mem.word(0xe64a) == 3, "fstat returned wrong size");
    check(call(sym("_fsync"), append, 0, "protected fsync") == 0,
          "fsync failed");
    call(sym("_close"), append, 0, "append close");
    files.observe = {};
    files.enabled = false;
    mem.observe = {};
    check(writes > 30 && checked_slot && !race,
          "kernel published shared state with interrupts enabled");
    check(cpu.snapshot().iff1 && !mem.bytes[sym("__interrupt_refcount")],
          "protected syscall leaked its critical section");

    // Audit both reads and writes: guarding the store alone cannot prevent
    // two threads from losing different pixels within one screen byte.
    unsigned screen_accesses = 0;
    mem.observe = [&](std::uint16_t address, bool) {
        if (address >= 0x4000 && address < 0x5800 && cpu.pc() < 0x4000) {
            ++screen_accesses;
            race |= cpu.snapshot().iff1;
        }
    };
    call(sym("_gpx_draw_pixel"), context, 8, "atomic pixel", 0,
         {10, 0, 1, 0, 0, 0});
    call(sym("_gpx_draw_line"), context, 8, "atomic diagonal", 0,
         {10, 0, 20, 0, 20, 0, 1, 0, 0xff, 0, 0});
    call(sym("_gpx_draw_line"), context, 8, "atomic horizontal span", 0,
         {10, 0, 50, 0, 10, 0, 1, 1, 0xff, 0, 0});
    call(sym("_gpx_draw_line"), context, 8, "atomic vertical line", 0,
         {10, 0, 8, 0, 25, 0, 0, 0, 0xaa, 0, 0});
    const std::uint8_t bitmap[] = {0, 8, 2, 2, 0, 0xaa, 0x55};
    std::copy(std::begin(bitmap), std::end(bitmap), mem.bytes.begin() + 0xe600);
    call(sym("_gpx_draw_bmp"), context, 9, "atomic bitmap rows", 0,
         {10, 0, 0, 0xe6, 0, 0});
    // sprite {x,y,bitmap,background,clip}: exercise save-under and raw blit.
    mem.word(0xe620, 9);
    mem.word(0xe622, 10);
    mem.word(0xe624, 0xe600);
    mem.word(0xe626, 0xe640);
    mem.word(0xe628, 0);
    call(sym("_gpx_show_sprite"), context, 0xe620, "atomic sprite save/draw");
    call(sym("_gpx_hide_sprite"), context, 0xe620, "atomic sprite restore");
    mem.observe = {};
    check(screen_accesses > 30 && !race,
          "GPX accessed a shared framebuffer byte without protection");
    check(cpu.snapshot().iff1 && !mem.bytes[sym("__interrupt_refcount")],
          "drawing left interrupts disabled");
}

#endif
