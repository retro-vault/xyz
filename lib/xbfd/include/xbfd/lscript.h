//
// xbfd/lscript.h — shared linker-script model and parsers.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace xbfd {

enum class lscript_mode {
    sdcc,
    gnu
};

enum class lscript_output_format {
    unknown,
    xl,
    bin,
    elf,
    ihx
};

struct lscript_address_range {
    uint16_t start = 0;
    uint16_t end = 0;
};

class lscript_error : public std::runtime_error {
public:
    explicit lscript_error(const std::string& message)
        : std::runtime_error(message) {}
};

class lscript {
public:
    virtual ~lscript() = default;

    const std::optional<std::string>& entry_symbol() const {
        return entry_symbol_;
    }
    const std::map<std::string, uint16_t>& area_bases() const {
        return area_bases_;
    }
    const std::vector<std::string>& area_order() const {
        return area_order_;
    }
    const std::vector<lscript_address_range>& reserved_ranges() const {
        return reserved_ranges_;
    }
    const std::optional<lscript_address_range>& output_range() const {
        return output_range_;
    }
    const std::optional<lscript_output_format>& output_format() const {
        return output_format_;
    }

    static std::unique_ptr<lscript> open(const std::filesystem::path& path,
                                         lscript_mode mode);
    void set_entry_symbol(std::string symbol) {
        entry_symbol_ = std::move(symbol);
    }
    void set_area_base(std::string area_name, uint16_t base) {
        area_bases_[std::move(area_name)] = base;
    }
    void add_area_order(std::string area_name) {
        for (const auto& existing : area_order_) {
            if (existing == area_name)
                return;
        }
        area_order_.push_back(std::move(area_name));
    }
    void add_reserved_range(lscript_address_range range) {
        reserved_ranges_.push_back(range);
    }
    void set_output_range(lscript_address_range range) {
        output_range_ = range;
    }
    void set_output_format(lscript_output_format format) {
        output_format_ = format;
    }

private:
    std::optional<std::string> entry_symbol_;
    std::map<std::string, uint16_t> area_bases_;
    std::vector<std::string> area_order_;
    std::vector<lscript_address_range> reserved_ranges_;
    std::optional<lscript_address_range> output_range_;
    std::optional<lscript_output_format> output_format_;
};

class gnu_lscript final : public lscript {
public:
    static std::unique_ptr<gnu_lscript> read(const std::filesystem::path& path);
};

class sdcc_lscript final : public lscript {
public:
    static std::unique_ptr<sdcc_lscript> read(const std::filesystem::path& path);
};

} // namespace xbfd
