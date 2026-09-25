// Shared-library lifetime, relocation and rollback regressions.
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
#ifndef YOS_TEST_LIBRARIES_H
#define YOS_TEST_LIBRARIES_H

#include <limits>
#include <stdexcept>

template<class Memory, class Cpu, class Call, class FarCall,
         class FarPointerCall, class FarRead, class Symbol, class Files,
         class Put>
void test_libraries(Memory& mem, Cpu& cpu, Call call, Symbol sym, Files& files,
                    const std::vector<std::uint8_t>& image,
                    const std::map<std::string, std::uint16_t>& exports,
                    Put put_string, FarCall far_call,
                    FarPointerCall far_pointer_call, FarRead far_read)
{
    const auto check = [](bool ok, const std::string& what) {
        if (!ok) throw std::runtime_error("library: " + what);
    };
    const auto original_thread = mem.word(sym("_thread_current"));
    const auto payload = unsigned(image[10] | (image[11] << 8));
    const auto error = [&] { return mem.bytes[sym("_process_last_error")]; };
    const auto heap_usage = [&](std::uint16_t head) {
        std::pair<unsigned, unsigned> result{};
        for (unsigned n = 0; head && n < 256; ++n, head = mem.word(head)) {
            if (mem.bytes[head + 4] & 1) {
                ++result.first;
                result.second += 7 + mem.word(head + 5);
            }
        }
        return result;
    };
    const auto clean_usage = std::make_pair(heap_usage(sym("__sys_heap")),
                                            heap_usage(0xc000));
    const auto usage = [&] {
        return std::make_pair(heap_usage(sym("__sys_heap")),
                              heap_usage(0xc000));
    };
    const auto create_client = [&](const std::string& name) {
        put_string(0xe200, name);
        const auto p = call(sym("_process_start"), 0xe200, 0x4102,
                            "create library client", 0, {128, 0});
        check(p != 0, "client creation failed");
        return p;
    };
    const auto retire_thread = [&](std::uint16_t thread) {
        call(sym("_list_remove"), sym("_thread_first_running"), thread,
             "unlink library client thread");
        call(sym("_list_insert"), sym("_thread_first_terminated"), thread,
             "terminate library client thread");
        mem.bytes[thread + 20] = 4;
        mem.word(sym("_thread_current"), original_thread);
        call(sym("__thread_cleanup_terminated"), 0, 0, "reap library client");
    };
    const auto current = [&](std::uint16_t p) {
        mem.word(sym("_thread_current"), mem.word(p + 14));
    };
    const auto load = [&](std::uint16_t flags = 1) {
        put_string(0xe240, "shelllib.svc");
        return call(sym("_library_load"), 0xe240, flags, "load library");
    };
    const auto query = [&] {
        put_string(0xe240, "shelllib");
        return call(sym("__svc_query"), 0xe240, 0, "query library");
    };
    const auto service_owner = [&](std::uint16_t table) {
        for (const auto root : {"__svc_first", "__library_private_services"}) {
            auto service = mem.word(sym(root));
            while (service) {
                if (mem.word(service + 21) == table)
                    return mem.word(service + 3);
                service = mem.word(service);
            }
        }
        return std::uint16_t(0);
    };
    const auto invoke = [&](std::uint16_t table, unsigned slot) {
        return far_call(std::uint16_t(table + 3 * slot), "library export");
    };
    files.enabled = true;
    auto a = create_client("client-a");
    auto b = create_client("client-b");
    current(a);
    // Actually preempt the first loader and have a second runnable thread
    // attempt the same load. It must get BUSY, then the first must complete
    // once, without inheriting that other thread's error or publishing early.
    const auto a_thread = mem.word(a + 14);
    const auto b_thread = mem.word(b + 14);
    const auto older_threads = mem.word(a_thread);
    mem.word(sym("_thread_first_running"), a_thread);
    mem.word(a_thread, b_thread);
    mem.word(b_thread, 0);
    constexpr std::uint16_t busy_entry = 0xe400, busy_result = 0xe480;
    put_string(0xe440, "shelllib.svc");
    const std::uint8_t contender[] = {
        0x21, 0x40, 0xe4,               // LD HL,path
        0x11, 0x01, 0x00,               // LD DE,SHARED
        0xcd, 0, 0,                     // CALL library_load
        0xed, 0x53, 0x80, 0xe4,         // LD (busy_result),DE
        0x3a, 0, 0,                     // LD A,(process_load_error)
        0x32, 0x82, 0xe4,               // LD (busy_result+2),A
        0x76, 0x18, 0xfd                // HALT; JR HALT
    };
    std::copy(std::begin(contender), std::end(contender),
              mem.bytes.begin() + busy_entry);
    mem.word(busy_entry + 7, sym("_library_load"));
    mem.word(busy_entry + 14, sym("_process_last_error"));
    mem.word(b_thread + 8, busy_entry);
    unsigned contention_phase = 0;
    files.observe = [&] {
        const auto state = cpu.snapshot();
        if (contention_phase == 0 && mem.bytes[sym("__image_busy")] &&
                mem.word(sym("_thread_current")) == a_thread && state.iff1) {
            check(cpu.interrupt(0xff), "could not preempt the active loader");
            contention_phase = 1;
        } else if (contention_phase == 1 && state.pc == busy_entry + 19) {
            check(mem.word(busy_result) == 0 && mem.bytes[busy_result + 2] == 9 &&
                      mem.bytes[sym("__image_busy")] == 1,
                  "concurrent loader did not return BUSY without unlocking");
            check(cpu.interrupt(0xff), "could not resume the original loader");
            contention_phase = 2;
        }
    };
    const auto first = load();
    files.observe = {};
    mem.word(sym("_thread_first_running"), b_thread);
    mem.word(b_thread, a_thread);
    mem.word(a_thread, older_threads);
    check(contention_phase == 2 && mem.bytes[b_thread + 16] == 9,
          "did not exercise both loader threads and save the BUSY error");
    check(first && error() == 0,
          "first shared load returned " + std::to_string(first) +
              " with error " + std::to_string(error()));
    const auto library = service_owner(first);
    check(library && mem.bytes[library + 5] == 3 &&
              mem.word(library + 14) == 1, "missing threadless library owner");
    const auto code = std::uint16_t(mem.word(first + 1) - exports.at("_probe"));
    // The callable table is common memory; executable code is retained in
    // its selected bank as a separate owner block.
    const auto code_size = unsigned(image[payload + 6] |
                                    (image[payload + 7] << 8));
    const auto table_block = std::uint16_t(first - 7);
    const auto code_block = std::uint16_t(code - 7);
    check((mem.bytes[table_block + 4] & 1) &&
              mem.word(table_block + 2) == library &&
              mem.word(table_block + 5) == 3 * 5,
          "resident library far table is not a common owner block");
    check((mem.bytes[code_block + 4] & 1) &&
              mem.word(code_block + 2) == library &&
              mem.word(code_block + 5) == code_size,
          "resident library code is not exactly one bank owner block");
    for (const auto [slot, name] :
         {std::pair{0, "_probe"}, {1, "_message"},
          {2, "_initializations"}, {3, "_calls"}, {4, "_sum3"}}) {
        const auto entry = std::uint16_t(first + 3 * slot);
        check(mem.bytes[entry] == mem.bytes[library + 16] &&
                  mem.word(entry + 1) == code + exports.at(name),
              "self-registered interface contains an invalid far pointer");
    }
    const auto storage = mem.word(code + exports.at("_storage"));
    check(storage && mem.word(storage - 5) == library,
          "initializer allocation belongs to the client instead of library");
    check(invoke(first, 2) == 1 && invoke(first, 3) == 0,
          "initializer/export counters are wrong");
    check(far_call(std::uint16_t(first + 12),
                   "three-argument far library call",
                   0x1000, 0x0200, 0x0034) == 0x1234,
          "three-argument far call returned the wrong sum");
    const auto message = far_pointer_call(std::uint16_t(first + 3),
                                          "library far-data export");
    const std::string expected_message = "Library OK";
    for (std::size_t i = 0; i <= expected_message.size(); ++i)
        check(far_read(message.first, std::uint16_t(message.second + i)) ==
                  std::uint8_t(expected_message.c_str()[i]),
              "library returned an invalid far string pointer");
    const auto resident_usage = heap_usage(sym("__sys_heap"));
    current(b);
    check(load() == first && mem.word(library + 14) == 2,
          "second process did not share the same image");
    check(load() == first && mem.word(library + 14) == 3,
          "repeated acquisition was not counted");
    const auto reused_usage = heap_usage(sym("__sys_heap"));
    check(reused_usage.first == resident_usage.first + 2 &&
              reused_usage.second == resident_usage.second + 2 * (7 + 7) &&
              invoke(first, 2) == 1,
          "shared reuse did not allocate exactly its two reference records, "
          "or initialized another image");
    retire_thread(mem.word(a + 14));
    check(mem.word(library + 14) == 2 && query() == first,
          "first client exit prematurely unloaded shared code");
    current(b);
    const auto private_table = load(0);
    check(private_table && private_table != first && query() == first,
          "private instance shared or shadowed the public service");
    check(invoke(private_table, 2) == 1 && invoke(first, 0) == 0x600d &&
              invoke(private_table, 0) == 0x600d &&
              invoke(first, 3) == 1 && invoke(private_table, 3) == 1,
          "private and shared state were not independent");
    files.files["shelllib.svc"][6] = 2;
    const auto other_abi = load();
    check(other_abi && other_abi != first && other_abi != private_table,
          "different image ABIs were incorrectly shared");
    files.files["shelllib.svc"] = image;
    const auto sibling = call(sym("_thread_create"), 0x4102, 128,
                              "create client sibling", 0,
                              {std::uint8_t(b), std::uint8_t(b >> 8)});
    check(sibling != 0, "sibling allocation failed");
    call(sym("_thread_resume"), sibling, 0, "resume client sibling");
    retire_thread(mem.word(b + 14));
    check(mem.word(library + 14) == 2,
          "main-thread exit released a still-live process's references");
    retire_thread(sibling);
    check(query() == 0 && mem.word(sym("__library_refs")) == 0 &&
              mem.word(sym("__library_private_services")) == 0,
          "last-thread exit left a library or service behind");
    check(usage() == clean_usage, "shared/private lifetime leaked heap storage");

    a = create_client("failures");
    current(a);
    const auto baseline = usage();
    const auto rollback = [&](const std::string& name, unsigned expected) {
        check(load() == 0 && error() == expected,
              name + ": expected error " + std::to_string(expected) +
                  ", got " + std::to_string(error()));
        check(usage() == baseline && files.handles.empty() &&
                  mem.word(sym("__library_refs")) == 0 &&
                  mem.word(sym("__library_private_services")) == 0 &&
                  mem.bytes[mem.word(a + 14) + 2] == 0xff &&
                  mem.word(mem.word(a + 14) + 3) == 0,
              name + ": failed load leaked resources or owner override");
        check(mem.bytes[sym("__image_busy")] == 0,
              name + ": failed load retained the loader lock");
        files.files["shelllib.svc"] = image;
    };
    const auto mutate = [&](unsigned offset, std::uint8_t value,
                             const std::string& name, unsigned expected = 4) {
        files.files["shelllib.svc"][offset] = value;
        rollback(name, expected);
    };
    // XL v2: 12-byte header, code, then the relocation table.
    const auto code_offset = payload + 12;
    const auto relocation_offset = code_offset + code_size;
    const auto checksum = [&] {
        auto& data = files.files["shelllib.svc"];
        std::uint32_t crc = 0xffffffff;
        for (std::size_t i = payload; i < data.size(); ++i) {
            crc ^= data[i];
            for (unsigned bit = 0; bit < 8; ++bit)
                crc = (crc >> 1) ^ ((crc & 1) ? 0xedb88320 : 0);
        }
        crc = ~crc;
        for (unsigned i = 0; i < 4; ++i) data[16 + i] = crc >> (8 * i);
    };
    mutate(0, 0, "bad magic");
    mutate(5, 1, "wrong image kind", 6);
    mutate(30, 4, "incompatible older ABI", 7);
    mutate(30, 7, "newer OS", 7);
    mutate(14, 1, "oversized payload");
    mutate(8, 0, "bad metadata size");
    mutate(7, 7, "unsupported fixed library");
    mutate(34, 0, "empty exports");
    mutate(35, 1, "too many exports");
    mutate(55, 1, "unterminated name");
    mutate(64, 0, "invalid export opcode");
    mutate(65, 0xff, "invalid export offset");
    mutate(26, 0xff, "invalid initializer offset");
    mutate(unsigned(image.size() - 1), 0xff, "bad payload CRC", 8);
    files.files["shelllib.svc"][relocation_offset + 2] = 3;
    checksum();
    rollback("invalid XL relocation", 4);
    files.files["shelllib.svc"][code_offset + exports.at("_init_status")] = 1;
    checksum();
    rollback("failure after allocation and self-registration", 11);
    files.files["shelllib.svc"].resize(50);
    rollback("truncated descriptor", 3);
    files.files["shelllib.svc"].resize(image.size() - 1);
    rollback("truncated payload", 3);
    files.fail_after = 64;
    files.read_error = true;
    rollback("native read error", 3);
    files.fail_after = std::numeric_limits<std::size_t>::max();
    files.read_error = false;
    files.files.erase("shelllib.svc");
    rollback("missing file", 1);
    mem.word(sym("_thread_current"), 0);
    check(load() == 0 && error() == 10, "ownerless load was accepted");
    current(a);
    mem.bytes[sym("__image_busy")] = 1;
    check(load() == 0 && error() == 9 && mem.bytes[sym("__image_busy")] == 1,
          "busy load altered the active loader lock");
    mem.bytes[sym("__image_busy")] = 0;
    check(load(2) == 0 && error() == 4, "unknown load flags accepted");

    // A service without an initializer uses the loader's compacted table.
    files.files["shelllib.svc"][7] = 4;
    files.files["shelllib.svc"][26] = 0xff;
    files.files["shelllib.svc"][27] = 0xff;
    const auto automatic = load(0);
    check(automatic && invoke(automatic, 2) == 0 &&
              invoke(automatic, 0) == 0x600d && query() == 0,
          "initializer-free private library did not bind its exports");
    files.files["shelllib.svc"] = image;
    retire_thread(mem.word(a + 14));
    check(usage() == clean_usage, "rollback tests leaked storage");

    // Exhaust each heap without relying on allocator block sizes.
    const auto fill_heap = [&](std::uint16_t heap) {
        std::vector<std::uint16_t> blocks;
        for (;;) {
            auto free_block = heap;
            while (free_block && (mem.bytes[free_block + 4] & 1))
                free_block = mem.word(free_block);
            if (!free_block) break;
            const auto block = call(sym("_mem_allocate"), heap,
                                    mem.word(free_block + 5),
                                    "fill heap for allocation failure",
                                    0, {0, 0});
            check(block != 0, "could not fill free heap block");
            blocks.push_back(block);
        }
        return blocks;
    };
    const auto empty_heap = [&](std::uint16_t heap, const auto& blocks) {
        for (const auto block : blocks)
            call(sym("_mem_free"), heap, block, "release heap filler");
    };
    a = create_client("oom");
    current(a);
    auto occupied = fill_heap(0xc000);
    check(load() == 0 && error() == 2 && files.handles.empty(),
          "image allocation failure did not close its descriptor");
    empty_heap(0xc000, occupied);
    occupied = fill_heap(sym("__sys_heap"));
    const auto full_system = usage();
    check(load() == 0 && error() == 2 && usage() == full_system,
          "library object allocation failure leaked the image");
    put_string(0xe240, "shell.sys");
    check(call(sym("_process_load"), 0xe240, 0, "process allocation failure") == 0 &&
              error() == 2 && usage() == full_system,
          "fixed-heap system-image allocation failure changed heap state");
    empty_heap(sym("__sys_heap"), occupied);
    const auto pinned = load();
    const auto pinned_owner = service_owner(pinned);
    check(pinned && pinned_owner, "could not prepare reference OOM test");
    occupied = fill_heap(sym("__sys_heap"));
    const auto before_reference_failure = usage();
    check(load() == 0 && error() == 2 &&
              mem.word(pinned_owner + 14) == 1 &&
              usage() == before_reference_failure,
          "reference allocation failure changed the resident library");
    empty_heap(sym("__sys_heap"), occupied);
    retire_thread(mem.word(a + 14));
    check(usage() == clean_usage, "out-of-memory paths leaked storage");
    check(files.opens == files.closes && files.handles.empty(),
          "a load path leaked file descriptors");
    files.enabled = false;
    mem.word(sym("_thread_current"), original_thread);
}

#endif
