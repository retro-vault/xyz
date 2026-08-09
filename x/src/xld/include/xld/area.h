// area.h
//
// linker area class
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
// 2021-07-28   tstih
#ifndef XLINK_AREA_HPP
#define XLINK_AREA_HPP

#include <cstdint>
#include <optional>
#include <string>

#include <xld/types.h>

namespace xld {

    class area {
    public:
        area() = default;

        area(const std::string& name, uint16_t size, area_flags flags,
             int index, std::optional<uint16_t> org_addr = std::nullopt)
            : name_(name), size_(size), flags_(flags),
              index_(index), org_addr_(org_addr) {}

        const std::string& name() const { return name_; }
        uint16_t size() const { return size_; }
        void set_size(uint16_t size) { size_ = size; }
        area_flags flags() const { return flags_; }
        int index() const { return index_; }
        std::optional<uint16_t> org_addr() const { return org_addr_; }

        // Set during linking.
        std::optional<uint16_t> placed_addr() const { return placed_addr_; }
        void set_placed_addr(uint16_t addr) { placed_addr_ = addr; }

        bool is_abs() const {
            return has_flag(flags_, area_flags::abs)
                || has_flag(flags_, area_flags::abs_legacy);
        }
        bool is_ovr() const { return has_flag(flags_, area_flags::ovr); }
        bool is_con() const { return !is_ovr(); }
        bool is_never_load() const {
            return has_flag(flags_, area_flags::never_load);
        }

    private:
        std::string name_;
        uint16_t size_ = 0;
        area_flags flags_ = area_flags::none;
        int index_ = 0;
        std::optional<uint16_t> org_addr_;
        std::optional<uint16_t> placed_addr_;
    };

} // namespace xld

#endif // XLINK_AREA_HPP
