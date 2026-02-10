// symbol.hpp
//
// linker symbol class
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
// 2021-07-28   tstih
#ifndef XLINK_SYMBOL_HPP
#define XLINK_SYMBOL_HPP

#include <cstdint>
#include <string>

#include <xlink/types.hpp>

namespace xlink {

    class module;

    class symbol {
    public:
        symbol() = default;

        symbol(const std::string& name, symbol_type type,
               uint16_t value, int index)
            : name_(name), type_(type), value_(value), index_(index) {}

        const std::string& name() const { return name_; }
        symbol_type type() const { return type_; }
        uint16_t value() const { return value_; }
        int index() const { return index_; }

        module* owner() const { return owner_; }
        void set_owner(module* m) { owner_ = m; }

        bool is_def() const { return type_ == symbol_type::def; }
        bool is_ref() const { return type_ == symbol_type::ref; }

    private:
        std::string name_;
        symbol_type type_ = symbol_type::ref;
        uint16_t value_ = 0;
        int index_ = 0;
        module* owner_ = nullptr;
    };

} // namespace xlink

#endif // XLINK_SYMBOL_HPP
