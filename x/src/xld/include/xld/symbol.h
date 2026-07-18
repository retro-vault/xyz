// symbol.h
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

#include <xld/types.h>

namespace xld {

    class module;

    class symbol {
    public:
        symbol() = default;

        symbol(const std::string& name, symbol_type type,
               uint16_t value, int index, int area_index = -1,
               bool absolute = false,
               symbol_kind kind = symbol_kind::notype,
               uint16_t size = 0,
               bool global = true,
               bool weak = false)
            : name_(name), type_(type), value_(value), index_(index),
              area_index_(area_index), absolute_(absolute), kind_(kind),
              size_(size), global_(global), weak_(weak) {}

        const std::string& name() const { return name_; }
        symbol_type type() const { return type_; }
        uint16_t value() const { return value_; }
        void set_value(uint16_t value) { value_ = value; }
        int index() const { return index_; }
        int area_index() const { return area_index_; }
        bool is_absolute() const { return absolute_; }
        void set_absolute(bool absolute) { absolute_ = absolute; }
        symbol_kind kind() const { return kind_; }
        void set_kind(symbol_kind kind) { kind_ = kind; }
        uint16_t size() const { return size_; }
        void set_size(uint16_t size) { size_ = size; }
        bool is_global() const { return global_; }
        bool is_local() const { return !global_; }
        void set_global(bool global) { global_ = global; }
        bool is_weak() const { return weak_; }
        bool is_strong() const { return !weak_; }
        void set_weak(bool weak) { weak_ = weak; }

        module* owner() const { return owner_; }
        void set_owner(module* m) { owner_ = m; }

        bool is_def() const { return type_ == symbol_type::def; }
        bool is_ref() const { return type_ == symbol_type::ref; }
        bool is_function() const { return kind_ == symbol_kind::function; }
        bool is_object() const { return kind_ == symbol_kind::object; }

    private:
        std::string name_;
        symbol_type type_ = symbol_type::ref;
        uint16_t value_ = 0;
        int index_ = 0;
        int area_index_ = -1;
        bool absolute_ = false;
        symbol_kind kind_ = symbol_kind::notype;
        uint16_t size_ = 0;
        bool global_ = true;
        bool weak_ = false;
        module* owner_ = nullptr;
    };

} // namespace xld

#endif // XLINK_SYMBOL_HPP
