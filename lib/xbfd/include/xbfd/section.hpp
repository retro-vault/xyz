// section.hpp
//
// bfd::section — mirrors GNU BFD's asection.  A section holds a name,
// flags, virtual memory address, raw content bytes, and a list of
// relocations that must be applied when the section is placed.
//
// Methods mirror the GNU BFD macro/function naming:
//   bfd_section_name   → section::name()
//   bfd_section_size   → section::size()
//   bfd_section_vma    → section::vma()
//   bfd_section_flags  → section::flags()
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#ifndef XBFD_SECTION_HPP
#define XBFD_SECTION_HPP

#include <cstdint>
#include <string>
#include <vector>

#include <xbfd/types.hpp>
#include <xbfd/reloc.hpp>

namespace bfd {

    class section {
    public:
        section() = default;

        section(const std::string& name, section_flags flags,
                uint64_t vma = 0)
            : name_(name), flags_(flags), vma_(vma) {}

        // -----------------------------------------------------------------------
        // Accessors (mirrors bfd_section_* macros / functions).
        // -----------------------------------------------------------------------

        const std::string& name()  const { return name_; }
        section_flags      flags() const { return flags_; }
        uint64_t           vma()   const { return vma_; }

        // Logical size: the declared area size (may exceed contents().size()
        // when the area reserves uninitialised space).
        uint64_t size() const {
            return size_ ? size_ : static_cast<uint64_t>(data_.size());
        }

        const std::vector<uint8_t>& contents() const { return data_; }
        const std::vector<reloc>&   relocs()   const { return relocs_; }

        // -----------------------------------------------------------------------
        // Mutators — used by writers and the assembler backend.
        // -----------------------------------------------------------------------

        void set_name(const std::string& n)     { name_  = n; }
        void set_flags(section_flags f)          { flags_ = f; }
        void set_vma(uint64_t v)                 { vma_   = v; }
        void set_size(uint64_t s)                { size_  = s; }
        void set_contents(std::vector<uint8_t> d){ data_  = std::move(d); }
        void append_byte(uint8_t b)              { data_.push_back(b); }
        void add_reloc(reloc r)                  { relocs_.push_back(std::move(r)); }

        // Patch a byte at offset (used during fixup passes).
        void patch_byte(uint32_t off, uint8_t v) { data_.at(off) = v; }
        void patch_word_le(uint32_t off, uint16_t v) {
            data_.at(off)     = static_cast<uint8_t>(v & 0xFF);
            data_.at(off + 1) = static_cast<uint8_t>(v >> 8);
        }

    private:
        std::string           name_;
        section_flags         flags_ = section_flags::none;
        uint64_t              vma_   = 0;
        uint64_t              size_  = 0;   // 0 = use data_.size()
        std::vector<uint8_t>  data_;
        std::vector<reloc>    relocs_;
    };

} // namespace bfd

#endif // XBFD_SECTION_HPP
