// RAM-gate esxDOS fixture: execute the real ROM loader and POSIX wrappers.
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
#ifndef YOS_TEST_MOCK_FILESYSTEM_H
#define YOS_TEST_MOCK_FILESYSTEM_H

#include <algorithm>
#include <functional>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

template<class Memory, class Cpu>
struct mock_filesystem {
    Memory& mem;
    Cpu& cpu;
    std::uint16_t open_gate, read_gate, close_gate;
    bool enabled = false;
    std::size_t max_read = 7; // Force the exact-read helper to loop.
    std::size_t fail_after = std::numeric_limits<std::size_t>::max();
    bool read_error = false;
    unsigned opens = 0, closes = 0;
    std::map<std::string, std::vector<std::uint8_t>> files;
    struct descriptor {
        std::string name;
        std::size_t position;
    };
    std::map<std::uint8_t, descriptor> handles;
    std::function<void()> observe;

    mock_filesystem(Memory& memory, Cpu& processor, std::uint16_t open,
                    std::uint16_t read, std::uint16_t close)
        : mem(memory), cpu(processor), open_gate(open), read_gate(read),
          close_gate(close)
    {
    }

    bool step()
    {
        if (!enabled) return false;
        if (observe) observe();
        const auto pc = cpu.pc();
        if (pc != open_gate && pc != read_gate && pc != close_gate)
            return false;
        auto state = cpu.snapshot();
        // The real firmware maps out the ROM containing the IM2 handler.
        if (state.iff1 || cpu.interrupt(0xff))
            throw std::runtime_error("esxDOS gate entered with IM2 enabled");
        auto handle = std::uint8_t(state.af >> 8);
        bool error = false;
        std::uint8_t result = 0;
        if (pc == open_gate) {
            std::string name;
            for (unsigned i = 0; i < 256; ++i) {
                const auto ch = mem.bytes[std::uint16_t(state.ix + i)];
                if (!ch) break;
                name += char(ch);
            }
            if (!files.contains(name)) {
                error = true;
                result = 5; // esxDOS ENOENT
            } else {
                for (handle = 32; handles.contains(handle); ++handle) {}
                handles.emplace(handle, descriptor{name, 0});
                result = handle;
                ++opens;
            }
        } else if (!handles.contains(handle)) {
            error = true;
            result = 13; // esxDOS EBADF
        } else if (pc == close_gate) {
            handles.erase(handle);
            ++closes;
        } else {
            auto& fd = handles.at(handle);
            const auto& data = files.at(fd.name);
            if (fd.position >= fail_after && read_error) {
                error = true;
                result = 6; // esxDOS EIO
            } else {
                const auto limit = std::min(data.size(), fail_after);
                const auto remaining = limit > fd.position ?
                    limit - fd.position : 0;
                const auto count = std::min({std::size_t(state.bc),
                                             max_read, remaining});
                for (std::size_t i = 0; i < count; ++i)
                    mem.write(std::uint16_t(state.ix + i),
                              data[fd.position + i]);
                fd.position += count;
                state.bc = std::uint16_t(count);
                state.ix += std::uint16_t(count);
            }
        }
        // Hostile firmware clobbers exercise the shared ROM adapter.
        state.iy = 0xd00d;
        state.hl = 0xabcd;
        state.de = 0xdcba;
        state.af = std::uint16_t(result << 8) | (error ? 1 : 0);
        state.pc = mem.word(state.sp);
        state.sp += 2;
        cpu.restore(state);
        return true;
    }
};

#endif
