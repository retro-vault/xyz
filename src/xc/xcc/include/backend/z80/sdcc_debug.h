//
// sdcc_debug.h — SDCC-compatible CDB debug info emitter for xcc.
//
// Emits:
//   - C$file.c$line$scope$block = . / .globl labels in the .s file so that
//     every source line has an address symbol xld can extract.
//   - G$name$0$0 = . / .globl before each function label (function address).
//   - G$name$0_0$0 = . before each global variable (variable address).
//   - A standard SDCC-format .adb file with M:, F:, S: records covering
//     functions, local variables, and global variables.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//

#pragma once
#include "backend/z80/debug_info.h"
#include "frontend/types.h"
#include <iosfwd>
#include <map>
#include <string>
#include <vector>

namespace xcc {

class sdcc_debug_emitter : public debug_info_emitter {
public:
    sdcc_debug_emitter(std::ostream &out,
                       const std::string &source_file,
                       const std::string &adb_path);

    void begin_module()                                    override;
    void emit_location(int line)                           override;
    void emit_global(const std::string &name,
                     const type *t,
                     bool is_static)                       override;
    void begin_function(const ir_function &fn,
                        const std::string &mangled)        override;
    void end_function(const ir_function &fn)               override;
    void end_module(const std::string &producer)           override;

private:
    // One source-line record for the .adb / C$ label.
    struct c_line { int line; int scope; int block; };

    // One local variable record collected during begin_function.
    struct local_var {
        std::string name;
        const type *t        = nullptr;
        int         offset   = 0;   // IX-relative offset (B storage)
        bool        on_stack = true;
    };

    // One function record stored for write_adb().
    struct fn_record {
        std::string  func_name;  // C name (no leading _)
        const type  *ret_type   = nullptr;
        bool         is_void    = false;
        int          block      = 7;    // block counter at start of this function
        std::vector<local_var> locals;
    };

    // One global variable record.
    struct gbl_record {
        std::string  name;
        const type  *t       = nullptr;
        bool         is_func = false;
        const type  *ret_t   = nullptr; // set when is_func
    };

    std::ostream        &out_;
    std::string          source_file_;
    std::string          module_name_;   // stem of source_file_
    std::string          adb_path_;

    int                  cur_line_    = -1;
    int                  block_ctr_   = 7;  // SDCC starts block numbering at 7

    std::vector<fn_record>  fn_records_;
    std::vector<gbl_record> gbl_records_;

    // Currently-open function (accumulated during begin_function … end_function).
    fn_record *cur_fn_ = nullptr;

    // ----- helpers -------------------------------------------------------
    static std::string module_stem(const std::string &source_file);

    // CDB type string {N}Tspec for S: records.
    static std::string cdb_type(const type *t);
    // CDB base type (no size prefix) for use inside derived types.
    static std::string cdb_base_type(const type *t);

    void write_adb() const;
};

} // namespace xcc
