// elf_emitter.cpp
//
// Emits ELF32 Z80 object files via libxbfd.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <xas/backend/emitter.h>
#include <xbfd/xbfd.h>

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
            obj_->set_module_name(name);
        }

        void set_default_calling_convention(xbfd::calling_convention cc) override
        {
            obj_->object().default_calling_convention = cc;
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
                sec->size += 1;
            }
            ++cur_offset_;
        }

        void emit_word(uint16_t v) override
        {
            auto* sec = obj_->find_section(cur_section_);
            if (sec) {
                sec->data.push_back(v & 0xFF);
                sec->data.push_back((v >> 8) & 0xFF);
                sec->size += 2;
            }
            cur_offset_ += 2;
        }

        void emit_space(uint32_t n, int) override
        {
            auto* sec = obj_->find_section(cur_section_);
            if (sec) {
                if (!bfd::has_flag(sec->flags, bfd::section_flags::never_load)) {
                    for (uint32_t i = 0; i < n; ++i)
                        sec->data.push_back(0);
                }
                sec->size += n;
            }
            cur_offset_ += n;
        }

        void emit_reloc(const std::string& name,
                         bfd::reloc_type type,
                         bool sym_relative,
                         int32_t addend) override
        {
            auto* sec = obj_->find_section(cur_section_);
            if (!sec) return;
            bfd::reloc r;
            r.offset       = cur_offset_;
            r.type         = type;
            r.sym_relative = sym_relative;
            r.name         = name;
            r.addend       = addend;
            sec->relocs.push_back(r);
        }

        bool gnu_pcrel8_relocations() const override { return true; }

        void define_symbol(const std::string& name,
                            uint64_t value,
                            const std::string& section_name,
                            bool global) override
        {
            if (defined_syms_.count(name)) {
                if (auto* sym = find_symbol(name)) {
                    sym->value = value;
                    sym->section_name = section_name;
                    sym->flags = merge_binding(sym->flags,
                        global || sym->is_global());
                }
                return;
            }
            defined_syms_.insert(name);
            bfd::symbol_flags sf = global
                ? (bfd::symbol_flags::global | bfd::symbol_flags::function)
                : bfd::symbol_flags::local;
            if (auto it = pending_type_flags_.find(name);
                it != pending_type_flags_.end()) {
                sf = replace_type_flags(sf, it->second);
            }
            auto* existing = find_symbol(name);
            auto& sym = existing
                ? *existing
                : obj_->add_symbol(name, sf, value, section_name);
            if (existing) {
                sym.flags = merge_binding(sym.flags, global || sym.is_global());
                if (auto it = pending_type_flags_.find(name);
                    it != pending_type_flags_.end()) {
                    sym.flags = replace_type_flags(sym.flags, it->second);
                }
                sym.value = value;
                sym.section_name = section_name;
            }
            if (auto it = pending_sizes_.find(name); it != pending_sizes_.end())
                sym.size = it->second;
        }

        void set_symbol_type(const std::string& name,
                             bfd::symbol_flags type_flags) override
        {
            pending_type_flags_[name] = type_flags;
            if (auto* sym = find_symbol(name))
                sym->flags = replace_type_flags(sym->flags, type_flags);
        }

        void set_symbol_size(const std::string& name, uint64_t size) override
        {
            pending_sizes_[name] = size;
            if (auto* sym = find_symbol(name))
                sym->size = size;
        }

        void mark_label(int) override {}

        void refer_symbol(const std::string& name) override
        {
            if (defined_syms_.count(name)) return;
            if (ref_syms_.count(name)) return;
            ref_syms_.insert(name);
            bfd::symbol_flags sf = bfd::symbol_flags::global
                                 | bfd::symbol_flags::undefined;
            if (auto it = pending_type_flags_.find(name);
                it != pending_type_flags_.end()) {
                sf = replace_type_flags(sf, it->second);
            }
            auto& sym = obj_->add_symbol(name, sf, 0, "");
            if (auto it = pending_sizes_.find(name); it != pending_sizes_.end())
                sym.size = it->second;
        }

        void end_module() override
        {
            obj_->close();
        }

        uint32_t current_offset() const override { return cur_offset_; }

    private:
        bfd::symbol* find_symbol(const std::string& name)
        {
            for (auto& sym : obj_->object().symbols) {
                if (sym.name == name)
                    return &sym;
            }
            return nullptr;
        }

        static bfd::symbol_flags replace_type_flags(
            bfd::symbol_flags flags,
            bfd::symbol_flags type_flags)
        {
            constexpr uint32_t type_mask =
                static_cast<uint32_t>(bfd::symbol_flags::function)
              | static_cast<uint32_t>(bfd::symbol_flags::object);
            uint32_t raw = static_cast<uint32_t>(flags) & ~type_mask;
            raw |= static_cast<uint32_t>(type_flags) & type_mask;
            return static_cast<bfd::symbol_flags>(raw);
        }

        static bfd::symbol_flags merge_binding(bfd::symbol_flags flags,
                                               bool global)
        {
            constexpr uint32_t bind_mask =
                static_cast<uint32_t>(bfd::symbol_flags::local)
              | static_cast<uint32_t>(bfd::symbol_flags::global)
              | static_cast<uint32_t>(bfd::symbol_flags::undefined);
            uint32_t raw = static_cast<uint32_t>(flags) & ~bind_mask;
            raw |= static_cast<uint32_t>(
                global ? bfd::symbol_flags::global : bfd::symbol_flags::local);
            return static_cast<bfd::symbol_flags>(raw);
        }

        std::string                output_path_;
        std::unique_ptr<bfd::bfd>  obj_;
        std::string                module_name_;
        std::string                cur_section_;
        uint32_t                   cur_offset_ = 0;
        std::unordered_set<std::string> defined_syms_;
        std::unordered_set<std::string> ref_syms_;
        std::unordered_map<std::string, bfd::symbol_flags> pending_type_flags_;
        std::unordered_map<std::string, uint64_t> pending_sizes_;
    };

    std::unique_ptr<emitter> make_elf_emitter(const std::string& path)
    {
        return std::make_unique<elf_emitter>(path);
    }

} // namespace xas
