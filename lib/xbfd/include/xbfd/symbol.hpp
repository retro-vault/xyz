// symbol.hpp
//
// bfd::symbol — mirrors GNU BFD's asymbol.  Each symbol has a name,
// flags, a section-relative or absolute value, and the name of the
// section it belongs to (empty string for undefined references or
// absolute symbols).
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#ifndef XBFD_SYMBOL_HPP
#define XBFD_SYMBOL_HPP

#include <cstdint>
#include <string>

#include <xbfd/types.hpp>

namespace bfd {

    class symbol {
    public:
        symbol() = default;

        symbol(const std::string& name, symbol_flags flags,
               uint64_t value, const std::string& section_name)
            : name_(name), flags_(flags), value_(value),
              section_name_(section_name) {}

        // -----------------------------------------------------------------------
        // Accessors.
        // -----------------------------------------------------------------------

        const std::string& name()         const { return name_; }
        symbol_flags       flags()        const { return flags_; }
        uint64_t           value()        const { return value_; }

        // Name of the owning section; empty for undefined or absolute.
        const std::string& section_name() const { return section_name_; }

        bool is_defined()   const {
            return !has_flag(flags_, symbol_flags::undefined);
        }
        bool is_global()    const {
            return has_flag(flags_, symbol_flags::global);
        }
        bool is_local()     const {
            return has_flag(flags_, symbol_flags::local);
        }
        bool is_absolute()  const {
            return has_flag(flags_, symbol_flags::absolute);
        }

        // -----------------------------------------------------------------------
        // Mutators.
        // -----------------------------------------------------------------------

        void set_flags(symbol_flags f)           { flags_        = f; }
        void set_value(uint64_t v)               { value_        = v; }
        void set_section_name(const std::string& s) { section_name_ = s; }

    private:
        std::string  name_;
        symbol_flags flags_        = symbol_flags::none;
        uint64_t     value_        = 0;
        std::string  section_name_;
    };

} // namespace bfd

#endif // XBFD_SYMBOL_HPP
