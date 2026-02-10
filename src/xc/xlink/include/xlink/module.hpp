// module.hpp
//
// linker module class (represents one .rel file)
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
// 2021-07-28   tstih
#ifndef XLINK_MODULE_HPP
#define XLINK_MODULE_HPP

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <xlink/types.hpp>
#include <xlink/area.hpp>
#include <xlink/symbol.hpp>
#include <xlink/text_record.hpp>

namespace xlink {

    class module {
    public:
        module() = default;

        module(const std::string& name, const std::filesystem::path& path)
            : name_(name), path_(path) {}

        const std::string& name() const { return name_; }
        void set_name(const std::string& n) { name_ = n; }

        const std::filesystem::path& path() const { return path_; }

        byte_order endian() const { return endian_; }
        void set_endian(byte_order bo) { endian_ = bo; }

        // Areas.
        std::vector<area>& areas() { return areas_; }
        const std::vector<area>& areas() const { return areas_; }

        area& area_by_index(int idx) { return areas_.at(idx); }
        const area& area_by_index(int idx) const { return areas_.at(idx); }

        // Symbols.
        std::vector<symbol>& symbols() { return symbols_; }
        const std::vector<symbol>& symbols() const { return symbols_; }

        symbol& symbol_by_index(int idx) { return symbols_.at(idx); }
        const symbol& symbol_by_index(int idx) const { return symbols_.at(idx); }

        // Collect undefined (ref) symbols.
        std::vector<std::string> undefined_symbols() const {
            std::vector<std::string> result;
            for (auto& s : symbols_)
                if (s.is_ref()) result.push_back(s.name());
            return result;
        }

        // Collect defined symbols.
        std::vector<const symbol*> defined_symbols() const {
            std::vector<const symbol*> result;
            for (auto& s : symbols_)
                if (s.is_def()) result.push_back(&s);
            return result;
        }

        // Text records.
        std::vector<text_record>& texts() { return texts_; }
        const std::vector<text_record>& texts() const { return texts_; }

    private:
        std::string name_;
        std::filesystem::path path_;
        byte_order endian_ = byte_order::little_endian;
        std::vector<area> areas_;
        std::vector<symbol> symbols_;
        std::vector<text_record> texts_;
    };

} // namespace xlink

#endif // XLINK_MODULE_HPP
