// emitter.h
//
// Abstract base class for xas object-file emitters.  Subclasses:
//   rel_emitter  — produces SDCC .rel format via libxbfd
//   elf_emitter  — produces ELF32 z80-elf format via libxbfd
//
// The assembler pipeline calls:
//   1. begin_module(name)
//   2. set_section(section_name, flags)   — switches current section
//   3. emit_byte(v) / emit_word(v)        — append data
//   4. emit_reloc(sym_or_sec, type)       — record pending relocation
//   5. define_symbol(name, value, global) — export / define symbol
//   6. refer_symbol(name)                 — declare external reference
//   7. end_module()                       — finish and write file
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#ifndef XAS_BACKEND_EMITTER_HPP
#define XAS_BACKEND_EMITTER_HPP

#include <cstdint>
#include <filesystem>
#include <string>

#include <xbfd/xbfd.h>

namespace xas {

    class emitter {
    public:
        virtual ~emitter() = default;

        //
        // Initialise output; must be called before any other method.
        //
        virtual void begin_module(const std::string& name) = 0;

        //
        // Record the default calling convention for the assembled module.
        //
        virtual void set_default_calling_convention(
            xbfd::calling_convention /*cc*/) {}

        //
        // Switch to / create the named section.  Subsequent emit_* calls
        // go into this section.
        //
        virtual void set_section(const std::string& name,
                                  bfd::section_flags flags) = 0;

        //
        // Emit a single byte or a 16-bit little-endian word.
        //
        virtual void emit_byte(uint8_t v) = 0;
        virtual void emit_word(uint16_t v) = 0;

        //
        // Reserve n bytes of uninitialised space (.ds / .space).
        //
        virtual void emit_space(uint32_t n, int source_line) = 0;

        //
        // Record a relocation at the current position.
        // sym_relative: true = reference to a named symbol,
        //               false = reference to a section/area.
        // addend: constant addend associated with the relocation target.
        //
        virtual void emit_reloc(const std::string& name,
                                  bfd::reloc_type type,
                                  bool sym_relative,
                                  int32_t addend = 0) = 0;

        //
        // GNU ELF Z80 PC-relative branch relocations encode an additional
        // byte-position bias. SDCC REL keeps the ASxxxx placeholder/addend
        // form, so codegen asks the emitter which policy to use.
        //
        virtual bool gnu_pcrel8_relocations() const { return false; }

        //
        // Define a symbol at the current section offset.
        //
        virtual void define_symbol(const std::string& name,
                                    uint64_t value,
                                    const std::string& section_name,
                                    bool global) = 0;

        //
        // Record a label boundary at the current section offset.
        // SDCC REL output uses these to break T/R records at label lines.
        //
        virtual void mark_label(int /*source_line*/) {}

        //
        // Declare an external (undefined) symbol reference.
        //
        virtual void refer_symbol(const std::string& name) = 0;

        //
        // Flush and write the output file.
        //
        virtual void end_module() = 0;

        //
        // Current section offset in bytes (advances with emit_*).
        //
        virtual uint32_t current_offset() const = 0;
    };

} // namespace xas

#endif // XAS_BACKEND_EMITTER_HPP
