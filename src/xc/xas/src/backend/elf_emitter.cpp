// elf_emitter.cpp
//
// Emits ELF32 Z80 object files via libxbfd.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <string>
#include <unordered_set>

#include <xas/backend/emitter.hpp>
#include <xbfd/bfd.hpp>

namespace xas {

    class elf_emitter final : public emitter {
    public:
        explicit elf_emitter(const std::string& output_path)
            : output_path_(output_path),
              obj_(bfd::bfd::open_w(output_path, bfd::flavour::elf))
        {}

        void begin_module(const std::string& name) override
        {
            module_name_ = name;
        }

        void set_section(const std::string& name,
                          bfd::section_flags flags) override
        {
            cur_section_ = name;
            cur_offset_  = 0;
            if (!obj_->find_section(name))
                obj_->add_section(name, flags);
        }

        void emit_byte(uint8_t v) override
        {
            auto* sec = obj_->find_section(cur_section_);
            if (sec) sec->append_byte(v);
            ++cur_offset_;
        }

        void emit_word(uint16_t v) override
        {
            auto* sec = obj_->find_section(cur_section_);
            if (sec) {
                sec->append_byte(v & 0xFF);
                sec->append_byte((v >> 8) & 0xFF);
            }
            cur_offset_ += 2;
        }

        void emit_space(uint32_t n) override
        {
            auto* sec = obj_->find_section(cur_section_);
            if (sec) {
                for (uint32_t i = 0; i < n; ++i) sec->append_byte(0);
            }
            cur_offset_ += n;
        }

        void emit_reloc(const std::string& name,
                         bfd::reloc_type type,
                         bool sym_relative) override
        {
            auto* sec = obj_->find_section(cur_section_);
            if (!sec) return;
            bfd::reloc r;
            r.offset       = cur_offset_;
            r.type         = type;
            r.sym_relative = sym_relative;
            r.name         = name;
            r.addend       = 0;
            sec->add_reloc(r);
        }

        void define_symbol(const std::string& name,
                            uint64_t value,
                            const std::string& section_name,
                            bool global) override
        {
            if (defined_syms_.count(name)) return;
            defined_syms_.insert(name);
            bfd::symbol_flags sf = global
                ? (bfd::symbol_flags::global | bfd::symbol_flags::function)
                : bfd::symbol_flags::local;
            obj_->add_symbol(name, sf, value, section_name);
        }

        void refer_symbol(const std::string& name) override
        {
            if (defined_syms_.count(name)) return;
            if (ref_syms_.count(name)) return;
            ref_syms_.insert(name);
            bfd::symbol_flags sf = bfd::symbol_flags::global
                                 | bfd::symbol_flags::undefined;
            obj_->add_symbol(name, sf, 0, "");
        }

        void end_module() override
        {
            obj_->close();
        }

        uint32_t current_offset() const override { return cur_offset_; }

    private:
        std::string                output_path_;
        std::unique_ptr<bfd::bfd>  obj_;
        std::string                module_name_;
        std::string                cur_section_;
        uint32_t                   cur_offset_ = 0;
        std::unordered_set<std::string> defined_syms_;
        std::unordered_set<std::string> ref_syms_;
    };

    std::unique_ptr<emitter> make_elf_emitter(const std::string& path)
    {
        return std::make_unique<elf_emitter>(path);
    }

} // namespace xas
