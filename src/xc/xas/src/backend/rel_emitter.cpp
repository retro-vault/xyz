// rel_emitter.cpp
//
// Emits SDCC .rel format object files via libxbfd.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <filesystem>
#include <stdexcept>
#include <string>
#include <unordered_set>

#include <xas/backend/emitter.h>
#include <xbfd/xbfd.h>

namespace xas {

    // Forward declaration.
    void emit_rel(const bfd::bfd& obj, std::ostream& out);

    class rel_emitter final : public emitter {
    public:
        explicit rel_emitter(const std::string& output_path)
            : output_path_(output_path),
              obj_(bfd::bfd::open_w(output_path, bfd::flavour::rel))
        {}

        void begin_module(const std::string& name) override
        {
            module_name_ = name;
            obj_->set_module_name(name);
        }

        void set_section(const std::string& name,
                          bfd::section_flags flags) override
        {
            cur_section_ = name;
            auto* sec = obj_->find_section(name);
            if (!sec) {
                sec = &obj_->add_section(name, flags);
            } else {
                sec->flags = flags;
            }
            cur_offset_ = static_cast<uint32_t>(sec->size);
        }

        void emit_byte(uint8_t v) override
        {
            auto* sec = obj_->find_section(cur_section_);
            if (sec) {
                sec->data.push_back(v);
                sec->size = sec->data.size();
            }
            ++cur_offset_;
        }

        void emit_word(uint16_t v) override
        {
            auto* sec = obj_->find_section(cur_section_);
            if (sec) {
                sec->data.push_back(v & 0xFF);
                sec->data.push_back((v >> 8) & 0xFF);
                sec->size = sec->data.size();
            }
            cur_offset_ += 2;
        }

        void emit_space(uint32_t n) override
        {
            auto* sec = obj_->find_section(cur_section_);
            if (sec) {
                for (uint32_t i = 0; i < n; ++i) sec->data.push_back(0);
                sec->size = sec->data.size();
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
            sec->relocs.push_back(r);
        }

        void define_symbol(const std::string& name,
                            uint64_t value,
                            const std::string& section_name,
                            bool global) override
        {
            if (defined_syms_.count(name)) return;
            defined_syms_.insert(name);
            bfd::symbol_flags sf = global ? bfd::symbol_flags::global
                                           : bfd::symbol_flags::local;
            if (section_name.empty())
                sf = sf | bfd::symbol_flags::absolute;
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

    // -------------------------------------------------------------------------
    // Factory
    // -------------------------------------------------------------------------

    std::unique_ptr<emitter> make_rel_emitter(const std::string& path)
    {
        return std::make_unique<rel_emitter>(path);
    }

} // namespace xas
