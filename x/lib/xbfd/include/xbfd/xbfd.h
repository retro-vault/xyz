//
// xbfd/xbfd.h — the one public header for the X Binary Format Driver.
//
// Three base-class pairs define the contract:
//   binary_reader / binary_writer  — raw encoded byte formats (ELF, iHEX, …)
//   di_reader     / di_writer      — symbolic debug information (CDB, DWARF2, …)
//   obj_reader    / obj_writer     — relocatable objects + archives (REL, AR, …)
//
// Concrete classes:
//   elf_reader    : binary_reader
//   elf_writer    : binary_writer
//   cdb           : di_reader, di_writer   (SDCC CDB — read files or stream emit)
//   map_reader    : di_reader              (SDCC MAP file reader)
//   dwarf2_writer : di_writer             (DWARF2 streaming emitter)
//   writer        : di_writer             (multicasts to N di_writer backends)
//   rel_reader    : obj_reader            (SDCC .rel)
//   rel_writer    : obj_writer
//   ar_reader     : obj_reader            (AR binary and text-index)
//   ar_writer     : obj_writer
//
// Legacy wrappers (backward compatible):
//   bfd::bfd           — original dispatcher, still works unchanged
//   xbfd::debug_writer — alias for xbfd::writer
//   xbfd::debug_reader — static read_cdb / read_map / merge helpers
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#pragma once

#include <cstdint>
#include <filesystem>
#include <istream>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace xbfd {

// ===========================================================================
// Enumerations
// ===========================================================================

enum class byte_order   { little_endian, big_endian };
enum class obj_format   { unknown, object, archive, core };
enum class obj_flavour  { unknown, rel, elf, ar_text, ar_binary };
enum class debug_lang   { unknown, c, assembly };
enum class var_storage  { unknown, stack, reg, external };
using storage = var_storage; // backward compat alias

enum class calling_convention : uint8_t {
    unknown         = 0x00,
    normal          = 0x01, // DW_CC_normal
    xcc_sdcccall0   = 0x40, // DWARF user range
    xcc_sdcccall1   = 0x41,
    xcc_z88dk_fastcall = 0x42,
    xcc_z88dk_callee   = 0x43,
    xcc_naked       = 0x44,
    xcc_interrupt   = 0x45,
    xcc_critical    = 0x46,
    xcc_z88dk_smallc = 0x47,
};

inline std::string_view to_string(calling_convention cc) {
    switch (cc) {
    case calling_convention::normal:             return "normal";
    case calling_convention::xcc_sdcccall0:      return "sdcccall(0)";
    case calling_convention::xcc_sdcccall1:      return "sdcccall(1)";
    case calling_convention::xcc_z88dk_smallc:   return "z88dk::smallc";
    case calling_convention::xcc_z88dk_fastcall: return "z88dk::fastcall";
    case calling_convention::xcc_z88dk_callee:   return "z88dk::callee";
    case calling_convention::xcc_naked:          return "sdcc::naked";
    case calling_convention::xcc_interrupt:      return "sdcc::interrupt";
    case calling_convention::xcc_critical:       return "sdcc::critical";
    default:                                     return "unknown";
    }
}

inline std::optional<calling_convention> parse_calling_convention(std::string_view text) {
    if (text == "normal")            return calling_convention::normal;
    if (text == "sdcccall(0)")       return calling_convention::xcc_sdcccall0;
    if (text == "sdcccall(1)")       return calling_convention::xcc_sdcccall1;
    if (text == "z88dk::smallc")     return calling_convention::xcc_z88dk_smallc;
    if (text == "z88dk::fastcall")   return calling_convention::xcc_z88dk_fastcall;
    if (text == "z88dk::callee")     return calling_convention::xcc_z88dk_callee;
    if (text == "sdcc::naked")       return calling_convention::xcc_naked;
    if (text == "sdcc::interrupt")   return calling_convention::xcc_interrupt;
    if (text == "sdcc::critical")    return calling_convention::xcc_critical;
    return std::nullopt;
}

enum class section_flags : uint32_t {
    none=0, alloc=1, load=2, code=4, data=8, readonly=16,
    reloc=32, debugging=64, never_load=128, abs=256, overlay=512
};
inline section_flags operator|(section_flags a, section_flags b) { return section_flags(uint32_t(a)|uint32_t(b)); }
inline section_flags operator&(section_flags a, section_flags b) { return section_flags(uint32_t(a)&uint32_t(b)); }
inline bool has_flag(section_flags v, section_flags f) { return (v&f)==f; }

enum class symbol_flags : uint32_t {
    none=0, local=1, global=2, undefined=4, function=8, object=16, weak=32, absolute=64
};
inline symbol_flags operator|(symbol_flags a, symbol_flags b) { return symbol_flags(uint32_t(a)|uint32_t(b)); }
inline symbol_flags operator&(symbol_flags a, symbol_flags b) { return symbol_flags(uint32_t(a)&uint32_t(b)); }
inline bool has_flag(symbol_flags v, symbol_flags f) { return (v&f)==f; }

enum class reloc_type : uint8_t { none=0, z80_8=1, z80_pc8=3, z80_16=4, z80_16_msb=5 };

// ===========================================================================
// Binary object types
// ===========================================================================

struct reloc_entry {
    uint32_t    offset       = 0;
    reloc_type  type         = reloc_type::none;
    bool        sym_relative = false;
    std::string name;
    int32_t     addend       = 0;
};

struct emitted_item {
    std::vector<uint8_t>        data;
    std::optional<reloc_entry>  reloc;
    bool                        label_marker = false;
    uint32_t                    reserve_bytes = 0;
    int                         source_line = 0;
};

struct section {
    std::string              name;
    section_flags            flags = section_flags::none;
    uint64_t                 vma   = 0;
    uint64_t                 size  = 0;
    std::vector<uint8_t>     data;
    std::vector<reloc_entry> relocs;
    std::vector<emitted_item> emitted_items;
};

struct symbol {
    std::string  name;
    symbol_flags flags        = symbol_flags::none;
    uint64_t     value        = 0;
    std::string  section_name;

    bool is_defined()  const { return !has_flag(flags, symbol_flags::undefined); }
    bool is_global()   const { return  has_flag(flags, symbol_flags::global);    }
    bool is_absolute() const { return  has_flag(flags, symbol_flags::absolute);  }
};

struct archive_member {
    std::string                name;
    std::string                path;
    std::optional<std::string> data;
};

// ===========================================================================
// Debug info types
// ===========================================================================

struct debug_source_file { uint32_t id=0; std::string path; debug_lang language=debug_lang::unknown; };
struct debug_function    {
    std::string name;
    uint32_t start=0, end=0, file_id=0, line=0;
    calling_convention convention = calling_convention::unknown;
};
struct debug_variable    { std::string name, parent; var_storage storage=var_storage::unknown;
                           int offset=0; std::string reg, type_name; bool is_param=false; };
struct debug_line        { uint32_t address=0, line=0, file_id=0; };
struct debug_symbol      { std::string name; uint32_t address=0; };

struct debug_info {
    std::vector<debug_source_file> files;
    std::vector<debug_function>    functions;
    std::vector<debug_variable>    variables;
    std::vector<debug_line>        lines;
    std::vector<debug_symbol>      symbols;

    bool empty() const { return functions.empty() && symbols.empty() && lines.empty(); }

    const debug_function*    function_at(uint32_t address) const;
    const debug_line*        line_at    (uint32_t address) const;
    const debug_source_file* file_by_id (uint32_t id)      const;
};

// ===========================================================================
// Type descriptor for debug emission
// ===========================================================================

struct type_ref {
    enum class kind { void_, char_, uchar, short_, ushort, int_, uint_,
                      long_, ulong, llong, ullong, float_, double_,
                      pointer, array, function, struct_, union_ };
    kind base=kind::void_; int size_bytes=0, array_count=0; std::string tag;

    static type_ref void_()          { type_ref t; t.base=kind::void_;   t.size_bytes=0;   return t; }
    static type_ref uchar_()         { type_ref t; t.base=kind::uchar;   t.size_bytes=1;   return t; }
    static type_ref int_()           { type_ref t; t.base=kind::int_;    t.size_bytes=2;   return t; }
    static type_ref ptr_()           { type_ref t; t.base=kind::pointer; t.size_bytes=2;   return t; }
    static type_ref array_(int n, int e) { type_ref t; t.base=kind::array; t.size_bytes=n*e; t.array_count=n; return t; }
};

// ===========================================================================
// Unified object — what every reader/writer works with
// ===========================================================================

struct object {
    std::string                  module_name;
    obj_format                   format  = obj_format::unknown;
    obj_flavour                  flavour = obj_flavour::unknown;
    byte_order                   endian  = byte_order::little_endian;
    std::vector<section>         sections;
    std::vector<symbol>          symbols;
    std::vector<archive_member>  members;
    debug_info                   debug;
    calling_convention           default_calling_convention = calling_convention::unknown;
};

// ===========================================================================
// Base class pairs
// ===========================================================================

class binary_reader {
public:
    virtual ~binary_reader() = default;
    virtual std::optional<object> read(const std::string& path) = 0;
};

class binary_writer {
public:
    virtual ~binary_writer() = default;
    virtual void write(const std::string& path, const object& obj) = 0;
};

class di_codec {
public:
    virtual ~di_codec() = default;
};

class di_reader : public virtual di_codec {
public:
    virtual ~di_reader() = default;
    virtual std::optional<debug_info> read(const std::string& path) = 0;
};

class di_writer : public virtual di_codec {
public:
    virtual ~di_writer() = default;
    virtual void on_module_begin  (const std::string& /*source_file*/)                               {}
    virtual void on_module_end    (const std::string& /*producer*/)                                  {}
    virtual void on_global        (const std::string& /*name*/, const type_ref& /*type*/, bool)      {}
    virtual void on_function_begin(const std::string& /*c_name*/, const std::string& /*mangled*/,
                                   bool /*is_global*/, const type_ref& /*return_type*/,
                                   calling_convention /*cc*/)                                         {}
    virtual void on_local         (const std::string& /*name*/, const type_ref& /*type*/,
                                   var_storage /*storage*/, int /*offset*/,
                                   const std::string& /*reg*/ = "")                                  {}
    virtual void on_function_end  (const std::string& /*c_name*/)                                    {}
    virtual void on_source_line   (int /*line*/)                                                      {}
    virtual void on_asm_line      (int /*line*/, uint32_t /*address*/)                               {}
};

class obj_reader {
public:
    virtual ~obj_reader() = default;
    virtual std::optional<object> read(const std::string& path) = 0;
};

class obj_writer {
public:
    virtual ~obj_writer() = default;
    virtual void write(const std::string& path, const object& obj) = 0;
};

// ===========================================================================
// Concrete classes — binary/ (raw encoded byte formats)
// ===========================================================================

class elf_reader final : public binary_reader {
public:
    std::optional<object> read(const std::string& path) override;
};

class elf_writer final : public binary_writer {
public:
    void write(const std::string& path, const object& obj) override;
};

// ===========================================================================
// Concrete classes — debug_info/ (symbolic debug information)
// ===========================================================================

// SDCC CDB: reads .cdb files (di_reader) and emits streaming CDB debug info (di_writer).
// Default-construct for read-only use; use the stream constructor for emission.
class cdb final : public di_reader, public di_writer {
public:
    cdb() = default;
    cdb(std::ostream& asm_out, const std::string& src, const std::string& adb_path);
    ~cdb() override = default;

    // di_reader
    std::optional<debug_info> read(const std::string& path) override;

    // di_writer (valid when constructed with the stream ctor)
    void on_module_end    (const std::string& producer) override;
    void on_global        (const std::string& name, const type_ref& t, bool is_static) override;
    void on_function_begin(const std::string& c, const std::string& m, bool g,
                           const type_ref& r, calling_convention cc) override;
    void on_local         (const std::string& n, const type_ref& t, var_storage s, int o,
                           const std::string& reg = "") override;
    void on_function_end  (const std::string& c) override;
    void on_source_line   (int l) override;

private:
    struct local_r { std::string name; type_ref type; var_storage sc_; int off; std::string reg; };
    struct fn_r    {
        std::string name;
        bool global;
        type_ref ret;
        calling_convention cc = calling_convention::unknown;
        int blk;
        std::vector<local_r> locals;
    };
    struct gbl_r   { std::string name; type_ref type; bool is_static; };
    struct fnsym_r { std::string name; type_ref ret; bool global; calling_convention cc = calling_convention::unknown; };

    std::ostream*         out_  = nullptr;
    std::string           src_, mod_, adb_;
    int                   line_ = -1, blk_ = 7;
    fn_r*                 cur_  = nullptr;
    std::vector<fn_r>     fns_;
    std::vector<gbl_r>    gbls_;
    std::vector<fnsym_r>  fsyms_;

    void write_adb() const;
};

class map_reader final : public di_reader {
public:
    std::optional<debug_info> read(const std::string& path) override;
    std::optional<debug_info> read(const std::string& path, debug_info base);
    static debug_info merge(debug_info base, const debug_info& supplement);
};

class dwarf2_writer final : public di_writer {
public:
    dwarf2_writer(std::ostream& asm_out, const std::string& src);
    ~dwarf2_writer() override = default;

    void on_module_begin  (const std::string& source_file) override;
    void on_module_end    (const std::string& producer)    override;
    void on_function_begin(const std::string& c, const std::string& m, bool g,
                           const type_ref& r, calling_convention cc) override;
    void on_function_end  (const std::string& c) override;
    void on_source_line   (int l) override;

private:
    struct fn_t { std::string c, m; bool g; calling_convention cc = calling_convention::unknown; };
    std::ostream&     out_;
    std::string       src_;
    int               cur_ = -1;
    std::vector<fn_t> fns_;

    void        str    (const std::string& s);
    static std::string el(const std::string& n);
    void abbrev ();
    void info   (const std::string& producer);
    void aranges();
};

// writer — multicasts all di_writer events to N backends simultaneously
class writer final : public di_writer {
public:
    ~writer() override = default;

    writer& add      (std::unique_ptr<di_writer> h) { backends_.push_back(std::move(h)); return *this; }
    writer& add_cdb  (std::ostream& o, const std::string& src, const std::string& adb);
    writer& add_dwarf(std::ostream& o, const std::string& src);

    void on_module_begin  (const std::string& s) override { for (auto& b:backends_) b->on_module_begin(s); }
    void on_module_end    (const std::string& p) override { for (auto& b:backends_) b->on_module_end(p); }
    void on_global        (const std::string& n, const type_ref& t, bool s) override { for (auto& b:backends_) b->on_global(n,t,s); }
    void on_function_begin(const std::string& c, const std::string& m, bool g,
                           const type_ref& r, calling_convention cc) override {
        for (auto& b:backends_) b->on_function_begin(c,m,g,r,cc);
    }
    void on_local         (const std::string& n, const type_ref& t, var_storage s, int o, const std::string& r) override { for (auto& b:backends_) b->on_local(n,t,s,o,r); }
    void on_function_end  (const std::string& c) override { for (auto& b:backends_) b->on_function_end(c); }
    void on_source_line   (int l) override { for (auto& b:backends_) b->on_source_line(l); }
    void on_asm_line      (int l, uint32_t a) override { for (auto& b:backends_) b->on_asm_line(l,a); }

    // Backward compat aliases
    writer& add_sdcc(std::ostream& o, const std::string& s, const std::string& a) { return add_cdb(o,s,a); }
    void begin_module()                                                                          { on_module_begin(""); }
    void end_module  (const std::string& p = "")                                                { on_module_end(p); }
    void add_global  (const std::string& n, const type_ref& t, bool s)                          { on_global(n,t,s); }
    void begin_function(const std::string& c,const std::string& m,bool g,
                        const type_ref& r, calling_convention cc = calling_convention::unknown) {
        on_function_begin(c,m,g,r,cc);
    }
    void add_local   (const std::string& n,const type_ref& t,var_storage s,int o,const std::string& r="") { on_local(n,t,s,o,r); }
    void end_function(const std::string& c)                                                      { on_function_end(c); }
    void source_line (int l)                                                                     { on_source_line(l); }
    void asm_line    (int l, uint32_t a)                                                         { on_asm_line(l,a); }

private:
    std::vector<std::unique_ptr<di_writer>> backends_;
};

using debug_writer = writer; // backward compat alias

// ===========================================================================
// Concrete classes — object/ (relocatable objects + archives)
// ===========================================================================

class rel_reader final : public obj_reader {
public:
    std::optional<object> read(const std::string& path) override;
};

class rel_writer final : public obj_writer {
public:
    void write(const std::string& path, const object& obj) override;
};

class ar_reader final : public obj_reader {
public:
    std::optional<object> read(const std::string& path) override;
};

class ar_writer final : public obj_writer {
public:
    void write(const std::string& path, const object& obj) override;
};

// ===========================================================================
// debug_reader — backward compat static helpers
// ===========================================================================

class debug_reader {
public:
    static std::optional<debug_info> read_cdb(const std::string& path);
    static std::optional<debug_info> read_map(const std::string& path, debug_info base = {});
    static debug_info merge(debug_info base, const debug_info& supplement);
};

// ===========================================================================
// Errors
// ===========================================================================

class bfd_error    : public std::runtime_error { using std::runtime_error::runtime_error; };
class format_error : public bfd_error {
    using bfd_error::bfd_error;
public:
    format_error(const std::string& path, int line, const std::string& msg)
        : bfd_error(path + ":" + std::to_string(line) + ": " + msg) {}
};
class io_error    : public bfd_error { using bfd_error::bfd_error; };
class reloc_error : public bfd_error { using bfd_error::bfd_error; };

} // namespace xbfd

// ===========================================================================
// bfd::bfd — legacy API, unchanged
// ===========================================================================

namespace bfd {
    using xbfd::section_flags;
    using xbfd::symbol_flags;
    using xbfd::reloc_type;
    using xbfd::byte_order;
    using xbfd::section;
    using xbfd::symbol;
    using xbfd::reloc_entry;
    using xbfd::archive_member;
    using xbfd::has_flag;
    using format       = xbfd::obj_format;
    using flavour      = xbfd::obj_flavour;
    using bfd_error    = xbfd::bfd_error;
    using format_error = xbfd::format_error;
    using io_error     = xbfd::io_error;
    using reloc        = xbfd::reloc_entry;

    class bfd {
    public:
        ~bfd() = default;
        bfd(const bfd&) = delete;  bfd& operator=(const bfd&) = delete;
        bfd(bfd&&) = default;      bfd& operator=(bfd&&) = default;

        static std::unique_ptr<bfd> open_r(const std::filesystem::path& path);
        static std::unique_ptr<bfd> open_r_stream(const std::string& name, std::istream& input);
        static std::unique_ptr<bfd> open_w(const std::filesystem::path& path,
                                            xbfd::obj_flavour fmt = xbfd::obj_flavour::elf);
        static std::unique_ptr<bfd> create_archive(const std::filesystem::path& path,
                                                    xbfd::obj_flavour fmt = xbfd::obj_flavour::ar_binary);

        bool              check_format(xbfd::obj_format f)  const;
        xbfd::obj_flavour get_flavour()                      const { return obj_.flavour; }
        xbfd::obj_format  get_format()                       const { return obj_.format;  }
        xbfd::byte_order  endian()                           const { return obj_.endian;  }

        const std::filesystem::path& filename()    const { return path_; }
        const std::string&           module_name() const { return obj_.module_name; }
        void set_module_name(const std::string& n)       { obj_.module_name = n; }

        const std::vector<xbfd::section>& sections() const { return obj_.sections; }
              std::vector<xbfd::section>& sections()        { return obj_.sections; }
        xbfd::section* find_section(const std::string& name);
        xbfd::section& add_section (const std::string& name, xbfd::section_flags flags, uint64_t vma=0);

        const std::vector<xbfd::symbol>& symbols() const { return obj_.symbols; }
              std::vector<xbfd::symbol>& symbols()        { return obj_.symbols; }
        xbfd::symbol& add_symbol(const std::string& name, xbfd::symbol_flags flags,
                                  uint64_t value, const std::string& section_name);

        const std::vector<xbfd::archive_member>& members() const { return obj_.members; }
        void add_member(xbfd::archive_member m) { obj_.members.push_back(std::move(m)); }

        void close();

        const xbfd::object& object() const { return obj_; }
              xbfd::object& object()        { return obj_; }

    private:
        bfd() = default;
        static bool is_ar_magic (const std::string& s);
        static bool is_elf_magic(const std::string& s);
        static bool is_rel_magic(const std::string& s);

        std::filesystem::path path_;
        bool                  writable_ = false;
        xbfd::object          obj_;
    };
} // namespace bfd
