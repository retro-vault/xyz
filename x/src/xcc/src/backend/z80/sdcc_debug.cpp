//
// sdcc_debug.cpp — SDCC-compatible CDB debug info emitter for xcc.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//

#include "backend/z80/sdcc_debug.h"
#include "ir/icode.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <ostream>
#include <sstream>

namespace xcc {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

sdcc_debug_emitter::sdcc_debug_emitter(std::ostream &out,
                                       const std::string &source_file,
                                       const std::string &adb_path)
    : out_(out)
    , source_file_(source_file)
    , module_name_(module_stem(source_file))
    , adb_path_(adb_path)
{}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

std::string sdcc_debug_emitter::module_stem(const std::string &source_file) {
    return std::filesystem::path(source_file).stem().string();
}

static std::string source_label_name(const std::string& source_file) {
    std::string name = std::filesystem::path(source_file).filename().string();
    if (name.empty())
        name = "source";

    for (char& ch : name) {
        const bool keep = (ch >= 'a' && ch <= 'z')
            || (ch >= 'A' && ch <= 'Z')
            || (ch >= '0' && ch <= '9')
            || ch == '_'
            || ch == '.';
        if (!keep)
            ch = '_';
    }
    return name;
}

// Return the CDB base type string (no {N} prefix).
// Used recursively for derived types (arrays, pointers, functions).
std::string sdcc_debug_emitter::cdb_base_type(const type *t) {
    if (!t) return "SV:S";
    switch (t->kind) {
    case type_kind::VOID:    return "SV:S";
    case type_kind::BOOL:    return "SC:U";
    case type_kind::CHAR:    return plain_char_is_unsigned() ? "SC:U" : "SC:S";
    case type_kind::SCHAR:   return "SC:S";
    case type_kind::UCHAR:   return "SC:U";
    case type_kind::CHAR8T:  return "SC:U";
    case type_kind::SHORT:   return "SI:S";
    case type_kind::USHORT:  return "SI:U";
    case type_kind::INT:     return "SI:S";
    case type_kind::UINT:    return "SI:U";
    case type_kind::ENUM:    return "SI:S";
    case type_kind::LONG:    return "SL:S";
    case type_kind::ULONG:   return "SL:U";
    case type_kind::LLONG:   return "SQ:S";
    case type_kind::ULLONG:  return "SQ:U";
    case type_kind::FLOAT:
    case type_kind::DOUBLE:
    case type_kind::COMPLEX: return "SF:S";
    case type_kind::POINTER:
        return "DP," + cdb_base_type(t->base.get());
    case type_kind::ARRAY: {
        int count = (t->array_size > 0 && t->base && t->base->size() > 0)
                    ? t->array_size / t->base->size()
                    : 1;
        return "DA" + std::to_string(count) + "d," + cdb_base_type(t->base.get());
    }
    case type_kind::FUNCTION:
        return "DF," + cdb_base_type(t->base.get());
    case type_kind::STRUCT:
        return "ST" + (t->tag.empty() ? "?" : t->tag) + ":S";
    case type_kind::UNION:
        return "SU" + (t->tag.empty() ? "?" : t->tag) + ":S";
    default:
        return "SI:S";
    }
}

// Return the full CDB type string {N}base.
std::string sdcc_debug_emitter::cdb_type(const type *t) {
    int sz = t ? t->size() : 2;
    if (sz <= 0) sz = 2;
    return "{" + std::to_string(sz) + "}" + cdb_base_type(t);
}

// ---------------------------------------------------------------------------
// debug_info_emitter interface
// ---------------------------------------------------------------------------

void sdcc_debug_emitter::begin_module() {
    // Emit a minimal file comment (kept for human readability; not used by tools).
    out_ << "; !FILE " << source_file_ << "\n";
}

void sdcc_debug_emitter::emit_location(int line) {
    if (line <= 0 || line == cur_line_ || !cur_fn_) return;
    cur_line_ = line;

    // Emit a C$ label that xas exports to the .rel symbol table.
    // xld extracts these to produce L:C$file$line$… records in the .cdb.
    // Use label colon syntax (not equate) so the symbol is a Def not Ref.
    //   C$sieve.c$12$1_0$7:
    //   .globl C$sieve.c$12$1_0$7
    const int blk = cur_fn_ ? cur_fn_->block : block_ctr_;
    const std::string sym = "C$" + source_label_name(source_file_)
                          + "$" + std::to_string(line)
                          + "$1_0$" + std::to_string(blk);
    out_ << sym << ":\n"
         << "\t.globl\t" << sym << "\n";
}

void sdcc_debug_emitter::emit_global(const std::string &name,
                                     const type *t,
                                     bool is_static)
{
    // Emit G$name$0_0$0: label so xld can resolve the global variable address.
    // This must appear BEFORE the variable label so both map to the same address.
    if (!is_static) {
        const std::string sym = "G$" + name + "$0_0$0";
        out_ << sym << ":\n";
    }

    // Record for .adb.
    gbl_record g;
    g.name    = name;
    g.t       = t;
    g.is_func = false;
    gbl_records_.push_back(g);
}

void sdcc_debug_emitter::begin_function(const ir_function &fn,
                                        const std::string & /*mangled*/)
{
    // Emit the function address label before the entry label.
    // SDCC uses G$name$0$0 (no _0_0); xld resolves function addresses via these.
    const std::string func_sym = "G$" + fn.name + "$0$0";
    out_ << func_sym << ":\n"
         << "\t.globl\t" << func_sym << "\n";

    // Allocate a new function record.
    fn_records_.emplace_back();
    cur_fn_       = &fn_records_.back();
    cur_fn_->func_name = fn.name;
    cur_fn_->ret_type  = fn.ret_type.get();
    cur_fn_->is_void   = (!fn.ret_type
                          || fn.ret_type->kind == type_kind::VOID);

    // Assign and advance the block counter for this function scope.
    cur_fn_->block = block_ctr_++;

    // ----- Collect local variables from icodes ---------------------------
    // We want every named, non-global, non-param SYMBOL operand.
    std::map<std::string, local_var> seen;

    auto collect = [&](const operand &op) {
        if (op.kind != operand_kind::SYMBOL) return;
        if (op.is_global || op.is_param)     return;
        if (op.name.empty())                 return;
        if (seen.count(op.name))             return;

        local_var lv;
        lv.name    = op.name;
        lv.t       = op.type.get();
        lv.offset  = op.stack_offset;
        lv.on_stack = true;
        seen[op.name] = lv;
    };

    for (const auto &ic : fn.icodes) {
        collect(ic.result);
        collect(ic.left);
        collect(ic.right);
    }

    // Preserve order: sort by offset (most-negative first = closest to IX).
    std::vector<local_var> locals_sorted;
    for (auto &[nm, lv] : seen)
        locals_sorted.push_back(lv);
    std::sort(locals_sorted.begin(), locals_sorted.end(),
              [](const local_var &a, const local_var &b) {
                  return a.offset < b.offset;
              });
    cur_fn_->locals = std::move(locals_sorted);

    // Record function as a global symbol for the function-symbol S: records.
    gbl_record g;
    g.name    = fn.name;
    g.t       = nullptr;
    g.is_func = true;
    g.ret_t   = fn.ret_type.get();
    gbl_records_.push_back(g);
}

void sdcc_debug_emitter::end_function(const ir_function &) {
    cur_fn_  = nullptr;
    cur_line_ = -1;
}

void sdcc_debug_emitter::end_module(const std::string &) {
    write_adb();
}

// ---------------------------------------------------------------------------
// .adb writer — emits standard SDCC CDB format
// ---------------------------------------------------------------------------

void sdcc_debug_emitter::write_adb() const {
    if (adb_path_.empty()) return;
    std::ofstream f(adb_path_);
    if (!f) return;

    // M: module record
    f << "M:" << module_name_ << "\n";

    // F: and local S: records for each function
    for (const auto &fn : fn_records_) {
        // Determine return type string
        const std::string ret_base = cdb_base_type(fn.ret_type);
        f << "F:G$" << fn.func_name << "$0_0$0"
          << "({2}DF," << ret_base << "),C,0,0,0,0,0\n";

        // S: records for local variables (B = IX-relative stack)
        for (const auto &lv : fn.locals) {
            const std::string type_str = cdb_type(lv.t);
            f << "S:L" << module_name_ << "." << fn.func_name
              << "$" << lv.name
              << "$1_0$" << fn.block
              << "(" << type_str << "),B,0," << lv.offset << "\n";
        }
    }

    // S:G records for global variables
    for (const auto &g : gbl_records_) {
        if (g.is_func) continue;
        const std::string type_str = cdb_type(g.t);
        f << "S:G$" << g.name << "$0_0$0"
          << "(" << type_str << "),E,0,0\n";
    }

    // S:G records for functions-as-symbols (at end of .adb like SDCC)
    for (const auto &g : gbl_records_) {
        if (!g.is_func) continue;
        const std::string ret_base = cdb_base_type(g.ret_t);
        f << "S:G$" << g.name << "$0_0$0"
          << "({2}DF," << ret_base << "),C,0,0\n";
    }
}

} // namespace xcc
