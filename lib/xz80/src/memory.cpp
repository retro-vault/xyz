// memory.cpp — flat_memory implementation.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
#include <algorithm>
#include <xz80/memory.hpp>

namespace xz80 {

flat_memory::flat_memory(uint8_t fill) noexcept
{
    data_.fill(fill);
}

uint8_t flat_memory::read(uint16_t addr) const noexcept
{
    return data_[addr];
}

void flat_memory::write(uint16_t addr, uint8_t val) noexcept
{
    data_[addr] = val;
}

void flat_memory::load(uint16_t origin, std::span<const uint8_t> src) noexcept
{
    for (size_t i = 0; i < src.size(); ++i) {
        uint32_t addr = static_cast<uint32_t>(origin) + i;
        if (addr >= 0x10000u) break;
        data_[addr] = src[i];
    }
}

const std::array<uint8_t, 0x10000>& flat_memory::data() const noexcept
{
    return data_;
}

} // namespace xz80
