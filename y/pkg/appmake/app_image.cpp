#include "appmake/app_image.h"

#include <algorithm>
#include <array>
#include <format>
#include <stdexcept>

#include "appmake/util.h"

namespace appmake {

namespace {

constexpr std::array<uint8_t, 4> k_magic_legacy = {'Y', 'A', 'P', 'Z'};

}  // namespace

snapshot_48 parse_sna48(const fs::path& path) {
    const auto bytes = read_file(path);
    if (bytes.size() != 49179) {
        throw std::runtime_error("only 48K .sna files are supported (expected 49179 bytes)");
    }

    snapshot_48 out;
    const uint8_t* h = bytes.data();

    out.i = h[0];
    out.hl_alt = rd16(h + 1);
    out.de_alt = rd16(h + 3);
    out.bc_alt = rd16(h + 5);
    out.af_alt = rd16(h + 7);
    out.hl = rd16(h + 9);
    out.de = rd16(h + 11);
    out.bc = rd16(h + 13);
    out.iy = rd16(h + 15);
    out.ix = rd16(h + 17);
    out.iff2 = (h[19] & 0x04) ? 1 : 0;
    out.r = h[20];
    out.af = rd16(h + 21);

    const uint16_t saved_sp = rd16(h + 23);
    out.im = h[25];
    out.border = h[26];
    out.ram.assign(bytes.begin() + 27, bytes.end());

    if (saved_sp < 0x4000 || saved_sp >= 0xffff) {
        throw std::runtime_error("snapshot SP is outside 48K RAM");
    }

    const std::size_t pc_off = static_cast<std::size_t>(saved_sp - 0x4000);
    if (pc_off + 1 >= out.ram.size()) {
        throw std::runtime_error("snapshot PC cannot be recovered from stack");
    }

    out.pc = rd16(out.ram.data() + pc_off);
    out.sp = static_cast<uint16_t>(saved_sp + 2);
    return out;
}

std::vector<uint8_t> build_snapshot_state(const snapshot_48& sna, uint16_t sp) {
    std::vector<uint8_t> out(32, 0);
    wr16(out, 0, sna.af);
    wr16(out, 2, sna.bc);
    wr16(out, 4, sna.de);
    wr16(out, 6, sna.hl);
    wr16(out, 8, sna.af_alt);
    wr16(out, 10, sna.bc_alt);
    wr16(out, 12, sna.de_alt);
    wr16(out, 14, sna.hl_alt);
    wr16(out, 16, sna.ix);
    wr16(out, 18, sna.iy);
    wr16(out, 20, sp);
    out[22] = sna.i;
    out[23] = sna.r;
    out[24] = sna.iff2;
    out[25] = sna.im;
    out[26] = sna.border;
    return out;
}

void validate_range(uint16_t load_addr, std::size_t size, uint16_t entry_addr) {
    if (size == 0) {
        throw std::runtime_error("empty payload is not allowed");
    }
    if (static_cast<uint32_t>(load_addr) + static_cast<uint32_t>(size) > 0x10000UL) {
        throw std::runtime_error("payload range exceeds 64K address space");
    }

    const uint32_t start = load_addr;
    const uint32_t end = start + static_cast<uint32_t>(size);
    if (entry_addr < start || entry_addr >= end) {
        throw std::runtime_error("entry address does not point into the payload range");
    }
}

std::vector<uint8_t> build_app(
    const app_header& header,
    const std::vector<uint8_t>& state,
    const std::vector<uint8_t>& payload
) {
    std::vector<uint8_t> out(k_header_size + state.size() + payload.size(), 0);

    std::copy(k_magic_legacy.begin(), k_magic_legacy.end(), out.begin());
    out[4] = k_app_version;
    out[5] = header.kind;
    out[6] = header.flags;
    out[7] = k_header_size;
    wr16(out, 8, header.load_addr);
    wr16(out, 10, header.entry_addr);
    wr16(out, 12, header.payload_size);
    wr16(out, 14, header.state_size);
    wr16(out, 16, header.stack_ptr);
    out[18] = header.tape_flag;
    out[19] = header.tape_checksum;

    std::copy(state.begin(), state.end(), out.begin() + k_header_size);
    std::copy(payload.begin(), payload.end(), out.begin() + k_header_size + state.size());
    return out;
}

}  // namespace appmake
