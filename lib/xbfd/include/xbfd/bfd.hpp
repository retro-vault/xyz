// bfd.hpp
//
// bfd::bfd — the central object-file / archive handle, mirroring GNU BFD's
// "bfd *" type.  Use the static factory methods open_r() / open_w() and
// create_archive() in place of bfd_openr / bfd_openw.
//
// Method-to-GNU-BFD mapping (selected):
//   bfd_openr(p,t)              → bfd::bfd::open_r(p)
//   bfd_openw(p,t)              → bfd::bfd::open_w(p, flavour)
//   bfd_close(b)                → destructor (RAII) or b.close()
//   bfd_check_format(b,fmt)     → b.check_format(fmt)
//   bfd_get_flavour(b)          → b.get_flavour()
//   bfd_get_filename(b)         → b.filename()
//   bfd_map_over_sections(b,…)  → for (auto* s : b.sections())
//   bfd_canonicalize_symtab     → b.symbols()
//   bfd_openr_next_archived_file→ b.members()
//
// The bfd::archive_member struct mirrors what you get when iterating
// a GNU ar archive: the member name, its raw bytes (when embedded), or
// an external path (for text-index libraries).
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#ifndef XBFD_XBFD_HPP
#define XBFD_XBFD_HPP

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <xbfd/types.hpp>
#include <xbfd/errors.hpp>
#include <xbfd/section.hpp>
#include <xbfd/symbol.hpp>

namespace bfd {

    // -------------------------------------------------------------------------
    // archive_member — one entry from an archive; mirrors a "bfd *" returned
    // by bfd_openr_next_archived_file.
    // -------------------------------------------------------------------------
    struct archive_member {
        std::string                  name;      // member filename
        std::filesystem::path        path;      // external path (text-index)
        std::optional<std::string>   data;      // raw content (ar binary)
    };

    // -------------------------------------------------------------------------
    // bfd — central handle
    // -------------------------------------------------------------------------
    class bfd {
    public:
        ~bfd() = default;

        // Disable copy; allow move.
        bfd(const bfd&)            = delete;
        bfd& operator=(const bfd&) = delete;
        bfd(bfd&&)                 = default;
        bfd& operator=(bfd&&)      = default;

        // -----------------------------------------------------------------------
        // Factories — mirrors bfd_openr / bfd_openw.
        // -----------------------------------------------------------------------

        //
        // Open an existing file for reading.  Format and flavour are detected
        // automatically from file content.
        //
        static std::unique_ptr<bfd> open_r(const std::filesystem::path& path);

        //
        // Parse a REL object directly from a stream (for embedded/archive use).
        //
        static std::unique_ptr<bfd> open_r_stream(const std::string& name,
                                                   std::istream& input);

        //
        // Create a new object file for writing.
        // flavour::rel  → SDCC .rel text format
        // flavour::elf  → ELF32 z80-elf
        //
        static std::unique_ptr<bfd> open_w(const std::filesystem::path& path,
                                            flavour fmt = flavour::elf);

        //
        // Create a new archive for writing.
        // flavour::ar_text   → SDCC text-index .lib
        // flavour::ar_binary → GNU ar .a
        //
        static std::unique_ptr<bfd> create_archive(
            const std::filesystem::path& path,
            flavour fmt = flavour::ar_binary);

        // -----------------------------------------------------------------------
        // Format queries — mirrors bfd_check_format / bfd_get_flavour.
        // -----------------------------------------------------------------------

        //
        // Returns true when this handle matches the requested format.
        // Must be called after open_r(); open_w() always returns format::object.
        //
        bool check_format(format fmt) const;

        //
        // Returns the detected on-disk encoding.
        //
        flavour     get_flavour() const { return flavour_; }
        format      get_format()  const { return format_; }
        byte_order  endian()      const { return endian_; }

        const std::filesystem::path& filename()    const { return path_; }
        const std::string&           module_name() const { return module_name_; }
        void set_module_name(const std::string& n)       { module_name_ = n; }

        // -----------------------------------------------------------------------
        // Object-file interface (valid when check_format(format::object)).
        // -----------------------------------------------------------------------

        //
        // All sections.  Mirrors bfd_map_over_sections.
        //
        const std::vector<section>& sections() const { return sections_; }
              std::vector<section>& sections()        { return sections_; }

        //
        // Find a section by name; returns nullptr if absent.
        //
        section* find_section(const std::string& name);

        //
        // Add a new section; returns reference to the stored section.
        //
        section& add_section(const std::string& name, section_flags flags,
                             uint64_t vma = 0);

        //
        // All symbols.  Mirrors bfd_canonicalize_symtab.
        //
        const std::vector<symbol>& symbols() const { return symbols_; }
              std::vector<symbol>& symbols()        { return symbols_; }

        //
        // Add a symbol; returns reference to the stored symbol.
        //
        symbol& add_symbol(const std::string& name, symbol_flags flags,
                           uint64_t value, const std::string& section_name);

        // -----------------------------------------------------------------------
        // Archive interface (valid when check_format(format::archive)).
        // -----------------------------------------------------------------------

        //
        // Returns all members.  Mirrors bfd_openr_next_archived_file iteration.
        //
        const std::vector<archive_member>& members() const { return members_; }

        //
        // Add a member to an archive opened for writing.
        //
        void add_member(archive_member m) { members_.push_back(std::move(m)); }

        // -----------------------------------------------------------------------
        // Write — flush to disk.  Must be called on objects opened via open_w /
        // create_archive.  Mirrors bfd_close for writable handles.
        // -----------------------------------------------------------------------

        void close();

    private:
        // Only factories construct instances.
        bfd() = default;

        std::filesystem::path      path_;
        std::string                module_name_;
        format                     format_   = format::unknown;
        flavour                    flavour_  = flavour::unknown;
        byte_order                 endian_   = byte_order::little_endian;
        bool                       writable_ = false;

        std::vector<section>       sections_;
        std::vector<symbol>        symbols_;
        std::vector<archive_member> members_;
    };

} // namespace bfd

#endif // XBFD_XBFD_HPP
