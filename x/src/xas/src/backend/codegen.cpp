// codegen.cpp
//
// Two-pass Z80 code generator.
//
// Pass 1: scan all statements, assign section offsets to labels, compute
//         instruction sizes.  Expressions with forward references are left
//         as "unknown" but their size is still determined.
//
// Pass 2: emit bytes to the emitter, resolve all expressions, patch
//         backward-known values and emit relocation records for symbols
//         that are still external after pass 1.
//
// Z80 register codes used throughout:
//   B=0  C=1  D=2  E=3  H=4  L=5  (HL)=6  A=7
//   BC=0 DE=1 HL=2 SP=3
//   BC=0 DE=1 HL=2 AF=3   (for PUSH/POP)
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <xas/errors.h>
#include <xas/cli.h>
#include <xas/frontend/ast.h>
#include <xas/backend/emitter.h>
#include <xbfd/xbfd.h>

namespace xas {

    // =========================================================================
    // Register encoding helpers
    // =========================================================================

    namespace {

        static std::optional<xbfd::calling_convention> parse_optsdcc_cc(const std::string& text)
        {
            std::string compact;
            compact.reserve(text.size());
            for (char ch : text) {
                if (ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n')
                    compact.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
            }
            if (compact.find("sdcccall(0)") != std::string::npos)
                return xbfd::calling_convention::xcc_sdcccall0;
            if (compact.find("sdcccall(1)") != std::string::npos)
                return xbfd::calling_convention::xcc_sdcccall1;
            if (compact.find("z88dk::fastcall") != std::string::npos)
                return xbfd::calling_convention::xcc_z88dk_fastcall;
            if (compact.find("z88dk::callee") != std::string::npos)
                return xbfd::calling_convention::xcc_z88dk_callee;
            return std::nullopt;
        }

        static std::string lowercase(std::string text)
        {
            for (char& ch : text)
                ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
            return text;
        }

        static bfd::section_flags classify_section_flags(const std::string& name)
        {
            const std::string lower = lowercase(name);
            bfd::section_flags flags = bfd::section_flags::alloc
                                     | bfd::section_flags::reloc;

            const bool is_bss_like =
                lower == ".bss" || lower == "_bss" || lower == "_heap";
            const bool is_data_like =
                lower == ".data" || lower == ".tdata"
                || lower == "_data" || lower == "_initialized"
                || lower == "_initializer" || lower == "_dabs";
            const bool is_rodata_like =
                lower == ".rodata" || lower == ".rdata"
                || lower == "_rodata" || lower == "_const";
            const bool is_code_like =
                lower == ".text" || lower == ".init" || lower == ".fini"
                || lower == "_code" || lower == "_gsinit"
                || lower == "_gsfinal" || lower == "_cabs"
                || lower == ".vectors";

            if (is_bss_like)
                return flags | bfd::section_flags::data
                             | bfd::section_flags::never_load;
            if (is_data_like)
                return flags | bfd::section_flags::load
                             | bfd::section_flags::data;
            if (is_rodata_like)
                return flags | bfd::section_flags::load
                             | bfd::section_flags::readonly;
            if (is_code_like)
                return flags | bfd::section_flags::load
                             | bfd::section_flags::code;

            return flags | bfd::section_flags::load
                         | bfd::section_flags::code;
        }

        // 8-bit register code (B=0..A=7, -1 if not a simple reg).
        static int reg8(const std::string& r)
        {
            if (r == "B") return 0;
            if (r == "C") return 1;
            if (r == "D") return 2;
            if (r == "E") return 3;
            if (r == "H") return 4;
            if (r == "L") return 5;
            if (r == "A") return 7;
            return -1;
        }

        // Undocumented but universally implemented Z80 index-register halves.
        // They occupy the normal H/L register-code slots behind DD/FD.  A
        // prefixed H or L operand would name the index half too, so transfers
        // involving an ordinary H/L register are not encodable and remain
        // rejected by the instruction selectors below.
        static bool index_half(const std::string& r, uint8_t& prefix, int& code)
        {
            if (r == "IXH") { prefix = 0xDD; code = 4; return true; }
            if (r == "IXL") { prefix = 0xDD; code = 5; return true; }
            if (r == "IYH") { prefix = 0xFD; code = 4; return true; }
            if (r == "IYL") { prefix = 0xFD; code = 5; return true; }
            return false;
        }

        // 16-bit register pair code for LD/INC/DEC/ADD (BC=0 DE=1 HL=2 SP=3).
        static int reg16_sp(const std::string& r)
        {
            if (r == "BC") return 0;
            if (r == "DE") return 1;
            if (r == "HL") return 2;
            if (r == "SP") return 3;
            return -1;
        }

        // 16-bit register pair code for PUSH/POP (BC=0 DE=1 HL=2 AF=3).
        static int reg16_af(const std::string& r)
        {
            if (r == "BC") return 0;
            if (r == "DE") return 1;
            if (r == "HL") return 2;
            if (r == "AF") return 3;
            return -1;
        }

        // Condition code (NZ=0 Z=1 NC=2 C=3 PO=4 PE=5 P=6 M=7).
        static int cond_code(const std::string& c)
        {
            if (c == "NZ") return 0;
            if (c == "Z")  return 1;
            if (c == "NC") return 2;
            if (c == "C")  return 3;
            if (c == "PO") return 4;
            if (c == "PE") return 5;
            if (c == "P")  return 6;
            if (c == "M")  return 7;
            return -1;
        }

        // Conditions valid for JR (subset: NZ=0, Z=1, NC=2, C=3).
        static int jr_cond(const std::string& c)
        {
            if (c == "NZ") return 0;
            if (c == "Z")  return 1;
            if (c == "NC") return 2;
            if (c == "C")  return 3;
            return -1;
        }

    } // anonymous namespace

    // =========================================================================
    // Symbol table
    // =========================================================================

    struct sym_entry {
        std::string section_name;
        uint32_t    value    = 0;
        bool        defined  = false;
        bool        global   = false;
        bool        external = false; // referenced but not defined
        xbfd::symbol_flags type_flags = xbfd::symbol_flags::none;
        uint64_t    size     = 0;
    };

    using sym_table = std::map<std::string, sym_entry>;

    // =========================================================================
    // Expression evaluator
    // =========================================================================

    static std::optional<int64_t> eval_expr(const expr& e,
                                             const sym_table& syms,
                                             uint32_t cur_addr)
    {
        switch (e.kind) {
            case expr_kind::integer:
                return e.int_val;

            case expr_kind::current_addr:
                return static_cast<int64_t>(cur_addr);

            case expr_kind::symbol: {
                auto it = syms.find(e.name);
                if (it == syms.end() || !it->second.defined)
                    return std::nullopt;
                return static_cast<int64_t>(it->second.value);
            }

            case expr_kind::unary: {
                auto v = eval_expr(*e.lhs, syms, cur_addr);
                if (!v) return std::nullopt;
                if (e.op == '-') return -*v;
                if (e.op == '~') return ~*v;
                return std::nullopt;
            }

            case expr_kind::binary: {
                auto lv = eval_expr(*e.lhs, syms, cur_addr);
                auto rv = eval_expr(*e.rhs, syms, cur_addr);
                if (!lv || !rv) return std::nullopt;
                switch (e.op) {
                    case '+': return *lv + *rv;
                    case '-': return *lv - *rv;
                    case '*': return *lv * *rv;
                    case '/': return *rv ? *lv / *rv : 0;
                    case '%': return *rv ? *lv % *rv : 0;
                    case '&': return *lv & *rv;
                    case '|': return *lv | *rv;
                    case '^': return *lv ^ *rv;
                    case '<': return *lv << *rv;
                    case '>': return *lv >> *rv;
                    default:  return std::nullopt;
                }
            }
        }
        return std::nullopt;
    }

    static std::optional<int64_t> eval_expr_symbol_zero(const expr& e,
                                                        const sym_table& syms,
                                                        uint32_t cur_addr)
    {
        switch (e.kind) {
            case expr_kind::integer:
                return e.int_val;

            case expr_kind::current_addr:
                return static_cast<int64_t>(cur_addr);

            case expr_kind::symbol: {
                auto it = syms.find(e.name);
                if (it != syms.end() && it->second.defined
                    && it->second.section_name.empty())
                    return static_cast<int64_t>(it->second.value);
                return 0;
            }

            case expr_kind::unary: {
                auto v = eval_expr_symbol_zero(*e.lhs, syms, cur_addr);
                if (!v) return std::nullopt;
                if (e.op == '-') return -*v;
                if (e.op == '~') return ~*v;
                return std::nullopt;
            }

            case expr_kind::binary: {
                auto lv = eval_expr_symbol_zero(*e.lhs, syms, cur_addr);
                auto rv = eval_expr_symbol_zero(*e.rhs, syms, cur_addr);
                if (!lv || !rv) return std::nullopt;
                switch (e.op) {
                    case '+': return *lv + *rv;
                    case '-': return *lv - *rv;
                    case '*': return *lv * *rv;
                    case '/': return *rv ? *lv / *rv : 0;
                    case '%': return *rv ? *lv % *rv : 0;
                    case '&': return *lv & *rv;
                    case '|': return *lv | *rv;
                    case '^': return *lv ^ *rv;
                    case '<': return *lv << *rv;
                    case '>': return *lv >> *rv;
                    default:  return std::nullopt;
                }
            }
        }
        return std::nullopt;
    }

    static bool is_relocatable_symbol(const std::string& name,
                                      const sym_table& syms)
    {
        auto it = syms.find(name);
        if (it == syms.end() || !it->second.defined)
            return true;
        return !it->second.section_name.empty();
    }

    static int symbol_ref_count(const expr& e, const sym_table& syms)
    {
        switch (e.kind) {
            case expr_kind::integer:
            case expr_kind::current_addr:
                return 0;
            case expr_kind::symbol:
                return is_relocatable_symbol(e.name, syms) ? 1 : 0;
            case expr_kind::unary:
                return symbol_ref_count(*e.lhs, syms);
            case expr_kind::binary:
                return symbol_ref_count(*e.lhs, syms)
                     + symbol_ref_count(*e.rhs, syms);
        }
        return 0;
    }

    static std::string reloc_symbol_name(const expr& e, const sym_table& syms);

    // Returns true if the expression should carry a relocation record.
    // For relocatable objects, even same-section absolute symbol references
    // such as `jp label` need relocation so the linker can add the section's
    // final base address. We only suppress relocations for fully absolute
    // expressions and for compound expressions with multiple symbol refs that
    // already fold to a stable constant (for example label differences).
    static bool needs_reloc(const expr& e, const sym_table& syms,
                             const std::string&)
    {
        const int sym_count = symbol_ref_count(e, syms);
        if (sym_count == 0)
            return false;
        if (sym_count != 1)
            return false;

        const auto name = reloc_symbol_name(e, syms);
        if (name.empty())
            return false;

        auto it = syms.find(name);
        if (it == syms.end())
            return true;
        if (!it->second.defined)
            return true;
        return !it->second.section_name.empty();
    }

    // PC-relative branches only need relocation when the target is not a
    // locally defined label in the same relocatable section. Same-section
    // deltas survive section rebasing unchanged.
    static bool needs_pcrel_reloc(const expr& e, const sym_table& syms,
                                  const std::string& cur_section)
    {
        const int sym_count = symbol_ref_count(e, syms);
        if (sym_count == 0)
            return false;
        if (sym_count != 1)
            return false;

        const auto name = reloc_symbol_name(e, syms);
        if (name.empty())
            return false;

        auto it = syms.find(name);
        if (it == syms.end())
            return true;
        if (!it->second.defined)
            return true;
        return it->second.section_name != cur_section;
    }

    static std::string reloc_symbol_name(const expr& e, const sym_table& syms)
    {
        if (e.kind == expr_kind::symbol)
            return is_relocatable_symbol(e.name, syms) ? e.name : "";
        if (e.kind == expr_kind::unary) return reloc_symbol_name(*e.lhs, syms);
        if (e.kind == expr_kind::binary) {
            std::string n = reloc_symbol_name(*e.lhs, syms);
            if (!n.empty()) return n;
            return reloc_symbol_name(*e.rhs, syms);
        }
        return {};
    }

    struct reloc_target {
        std::string name;
        bool sym_relative = true;
        bool valid = false;
    };

    static reloc_target pick_reloc_target(const expr& e, const sym_table& syms,
                                          const std::string&)
    {
        const auto name = reloc_symbol_name(e, syms);
        if (name.empty())
            return {};

        auto it = syms.find(name);
        if (it == syms.end() || !it->second.defined)
            return {name, true, true};

        if (!it->second.section_name.empty())
            return {it->second.section_name, false, true};

        return {};
    }

    static uint32_t reloc_addend(const expr& e, const sym_table& syms, uint32_t cur_offset,
                                 const reloc_target& target)
    {
        auto value = eval_expr(e, syms, cur_offset);
        auto relaxed = value ? value : eval_expr_symbol_zero(e, syms, cur_offset);
        if (!relaxed)
            return 0;

        const auto name = reloc_symbol_name(e, syms);
        if (name.empty() || !target.valid)
            return static_cast<uint32_t>(*relaxed);

        if (!target.sym_relative)
            return static_cast<uint32_t>(*relaxed);

        auto it = syms.find(name);
        if (it == syms.end() || !it->second.defined)
            return static_cast<uint32_t>(*relaxed);

        return static_cast<uint32_t>(*relaxed) - it->second.value;
    }

    static bool is_external_target(const reloc_target& target, const sym_table& syms)
    {
        if (!target.valid || !target.sym_relative || target.name.empty())
            return false;
        auto it = syms.find(target.name);
        return it == syms.end() || !it->second.defined;
    }

    // =========================================================================
    // codegen class
    // =========================================================================

    struct codegen {
        emitter&   emit_;
        sym_table  syms_;
        std::map<std::string, bool> emitted_sections_;
        std::map<std::string, uint32_t> section_offsets_;
        std::string cur_section_;
        uint32_t   cur_offset_ = 0;
        int        pass_       = 1;
        std::string src_file_;
        bool       section_ready_ = false;

        codegen(emitter& e, const std::string& src)
            : emit_(e), src_file_(src) {}

        void install_predefines(const std::vector<std::string>& defines)
        {
            for (const std::string& def : defines) {
                size_t eq = def.find('=');
                std::string name = eq == std::string::npos
                    ? def
                    : def.substr(0, eq);
                std::string text = eq == std::string::npos
                    ? "1"
                    : def.substr(eq + 1);
                if (name.empty())
                    throw codegen_error(src_file_, 0, "empty -D symbol");

                int64_t value = 0;
                try {
                    size_t consumed = 0;
                    value = std::stoll(text, &consumed, 0);
                    if (consumed != text.size())
                        throw std::invalid_argument("trailing characters");
                } catch (...) {
                    throw codegen_error(src_file_, 0,
                        "non-integer -D value for '" + name + "': " + text);
                }

                syms_[name].value = static_cast<uint32_t>(value);
                syms_[name].defined = true;
            }
        }

        bool conditional_symbol_defined(const stmt& s) const
        {
            if (s.args.empty() || s.args[0]->kind != expr_kind::symbol)
                throw codegen_error(s.source_file.empty() ? src_file_ : s.source_file,
                    s.source_line, "." + s.directive_name + " expects a symbol");
            auto it = syms_.find(s.args[0]->name);
            return it != syms_.end() && it->second.defined;
        }

        bool conditional_value(const stmt& s)
        {
            const std::string file =
                s.source_file.empty() ? src_file_ : s.source_file;
            if (s.directive_name == "ifdef")
                return conditional_symbol_defined(s);
            if (s.directive_name == "ifndef")
                return !conditional_symbol_defined(s);
            if (s.args.empty())
                throw codegen_error(file, s.source_line,
                    ".if expects an absolute expression");
            auto value = eval_expr(*s.args[0], syms_, cur_offset_);
            if (!value)
                throw codegen_error(file, s.source_line,
                    ".if expression is not an absolute known value");
            return *value != 0;
        }

        std::vector<const stmt*> filter_conditionals(const stmt_list& stmts)
        {
            struct frame {
                bool parent_active = true;
                bool condition_true = false;
                bool in_else = false;
                bool active = true;
            };

            std::vector<frame> stack;
            std::vector<const stmt*> active;

            auto current_active = [&]() {
                return stack.empty() ? true : stack.back().active;
            };

            for (const stmt& s : stmts) {
                if (s.kind == stmt_kind::directive
                    && (s.directive_name == "if"
                        || s.directive_name == "ifdef"
                        || s.directive_name == "ifndef")) {
                    const bool parent = current_active();
                    const bool condition = parent ? conditional_value(s) : false;
                    stack.push_back({ parent, condition, false,
                                      parent && condition });
                    continue;
                }

                if (s.kind == stmt_kind::directive
                    && s.directive_name == "else") {
                    if (stack.empty())
                        throw codegen_error(
                            s.source_file.empty() ? src_file_ : s.source_file,
                            s.source_line, ".else without matching .if");
                    frame& top = stack.back();
                    if (top.in_else)
                        throw codegen_error(
                            s.source_file.empty() ? src_file_ : s.source_file,
                            s.source_line, "duplicate .else");
                    top.in_else = true;
                    top.active = top.parent_active && !top.condition_true;
                    continue;
                }

                if (s.kind == stmt_kind::directive
                    && s.directive_name == "endif") {
                    if (stack.empty())
                        throw codegen_error(
                            s.source_file.empty() ? src_file_ : s.source_file,
                            s.source_line, ".endif without matching .if");
                    stack.pop_back();
                    continue;
                }

                if (current_active()) {
                    if (s.kind == stmt_kind::equ && s.equ_value) {
                        auto v = eval_expr(*s.equ_value, syms_, cur_offset_);
                        if (v) {
                            syms_[s.equ_name].value   = static_cast<uint32_t>(*v);
                            syms_[s.equ_name].defined = true;
                        }
                    } else if (s.kind == stmt_kind::directive) {
                        const std::string& dn = s.directive_name;
                        if ((dn == "equ" || dn == "set" || dn == "define")
                            && !s.string_arg.empty() && !s.args.empty()) {
                            auto v = eval_expr(*s.args[0], syms_, cur_offset_);
                            if (v) {
                                syms_[s.string_arg].value   = static_cast<uint32_t>(*v);
                                syms_[s.string_arg].defined = true;
                            }
                        }
                    }
                    active.push_back(&s);
                }
            }

            if (!stack.empty())
                throw codegen_error(src_file_, 0, "unterminated .if block");

            return active;
        }

        void note_section_offset()
        {
            section_offsets_[cur_section_] = cur_offset_;
        }

        uint32_t alignment_padding(uint32_t boundary, uint32_t at) const
        {
            if (boundary <= 1)
                return 0;
            const uint32_t rem = at % boundary;
            return rem == 0 ? 0 : boundary - rem;
        }

        uint32_t directive_alignment_boundary(const stmt& s)
        {
            if (s.directive_name == "p2align") {
                if (s.args.empty())
                    return 1;
                auto v = eval_expr(*s.args[0], syms_, cur_offset_);
                if (!v)
                    return 1;
                if (*v < 0 || *v > 30)
                    throw codegen_error(src_file_, s.source_line,
                                        ".p2align exponent out of range");
                return static_cast<uint32_t>(1u << static_cast<uint32_t>(*v));
            }

            if (s.args.empty())
                return 1;
            auto v = eval_expr(*s.args[0], syms_, cur_offset_);
            if (!v)
                return 1;
            if (*v < 0)
                throw codegen_error(src_file_, s.source_line,
                                    "." + s.directive_name +
                                    " alignment must be non-negative");
            return static_cast<uint32_t>(*v);
        }

        uint32_t directive_alignment_padding(const stmt& s)
        {
            uint32_t n = alignment_padding(directive_alignment_boundary(s), cur_offset_);
            if (s.args.size() >= 3) {
                auto max_skip = eval_expr(*s.args[2], syms_, cur_offset_);
                if (max_skip && *max_skip >= 0
                    && n > static_cast<uint32_t>(*max_skip))
                    n = 0;
            }
            return n;
        }

        std::optional<uint8_t> directive_fill_byte(const stmt& s) const
        {
            if (s.args.size() < 2)
                return std::nullopt;
            auto v = eval_expr(*s.args[1], syms_, cur_offset_);
            if (!v)
                return std::nullopt;
            return static_cast<uint8_t>(*v);
        }

        xbfd::symbol_flags symbol_type_flags(std::string text) const
        {
            text.erase(std::remove_if(text.begin(), text.end(),
                                      [](unsigned char ch) {
                                          return ch == '@' || ch == '%'
                                              || ch == '#'
                                              || std::isspace(ch) != 0;
                                      }),
                       text.end());
            text = lowercase(text);
            if (text == "function" || text == "func")
                return xbfd::symbol_flags::function;
            if (text == "object" || text == "data")
                return xbfd::symbol_flags::object;
            return xbfd::symbol_flags::none;
        }

        void emit_fill_or_space(uint32_t n, std::optional<uint8_t> fill,
                                int source_line)
        {
            if (pass_ == 2) {
                ensure_section();
                if (fill) {
                    for (uint32_t i = 0; i < n; ++i)
                        emit_.emit_byte(*fill);
                } else {
                    emit_.emit_space(n, source_line);
                }
            }
            cur_offset_ += n;
            note_section_offset();
        }

        void switch_section(const std::string& name)
        {
            cur_section_ = name;
            cur_offset_ = section_offsets_[cur_section_];
            if (pass_ == 2) {
                bfd::section_flags sf = classify_section_flags(cur_section_);
                emit_.set_section(cur_section_, sf);
                emitted_sections_[cur_section_] = true;
                section_ready_ = true;
            } else {
                section_ready_ = false;
            }
        }

        // -----------------------------------------------------------------------
        // Emit helpers (only called in pass 2)
        // -----------------------------------------------------------------------

        void ensure_section()
        {
            if (pass_ != 2 || section_ready_)
                return;
            bfd::section_flags sf = classify_section_flags(cur_section_);
            emit_.set_section(cur_section_, sf);
            emitted_sections_[cur_section_] = true;
            section_ready_ = true;
        }

        void emit_byte_val(uint8_t v)
        {
            ensure_section();
            if (pass_ == 2) emit_.emit_byte(v);
            ++cur_offset_;
            note_section_offset();
        }

        void emit_word_val(uint16_t v)
        {
            ensure_section();
            if (pass_ == 2) emit_.emit_word(v);
            cur_offset_ += 2;
            note_section_offset();
        }

        // Emit a byte that may need a relocation.
        void emit_byte_expr(const expr& e, int)
        {
            auto v = eval_expr(e, syms_, cur_offset_);
            if (pass_ == 2) {
                ensure_section();
                if (needs_reloc(e, syms_, cur_section_)) {
                    const auto target = pick_reloc_target(e, syms_, cur_section_);
                    const auto addend = reloc_addend(e, syms_, cur_offset_, target);
                    if (is_external_target(target, syms_))
                        emit_.refer_symbol(target.name);
                    emit_.emit_reloc(target.name,
                                     bfd::reloc_type::z80_8,
                                     target.sym_relative,
                                     static_cast<int32_t>(addend));
                    emit_.emit_byte(static_cast<uint8_t>(addend & 0xFF));
                } else {
                    emit_.emit_byte(v ? static_cast<uint8_t>(*v) : 0);
                }
            }
            ++cur_offset_;
            note_section_offset();
        }

        // Emit a 16-bit word that may need a relocation.
        void emit_word_expr(const expr& e, int)
        {
            auto v = eval_expr(e, syms_, cur_offset_);
            if (pass_ == 2) {
                ensure_section();
                if (needs_reloc(e, syms_, cur_section_)) {
                    const auto target = pick_reloc_target(e, syms_, cur_section_);
                    const auto addend = reloc_addend(e, syms_, cur_offset_, target);
                    if (is_external_target(target, syms_))
                        emit_.refer_symbol(target.name);
                    emit_.emit_reloc(target.name,
                                     bfd::reloc_type::z80_16,
                                     target.sym_relative,
                                     static_cast<int32_t>(addend));
                    emit_.emit_word(static_cast<uint16_t>(addend & 0xFFFF));
                } else {
                    emit_.emit_word(v ? static_cast<uint16_t>(*v) : 0);
                }
            }
            cur_offset_ += 2;
            note_section_offset();
        }

        // Emit an 8-bit PC-relative offset (for JR / DJNZ).
        void emit_pcrel8(const expr& e, int line)
        {
            auto v = eval_expr(e, syms_, cur_offset_);
            if (pass_ == 2) {
                ensure_section();
                if (needs_pcrel_reloc(e, syms_, cur_section_)) {
                    const auto target = pick_reloc_target(e, syms_, cur_section_);
                    const auto target_addend =
                        static_cast<int32_t>(reloc_addend(e, syms_, cur_offset_, target));
                    auto reloc_addend = target_addend;
                    auto encoded_addend = target_addend;
                    if (emit_.gnu_pcrel8_relocations()) {
                        // ELF Z80 PC-relative relocations are applied at the
                        // displacement byte.  Z80 branches are relative to the
                        // following instruction, so unresolved targets need the
                        // same -1 bias GNU as emits for `jr external`.
                        reloc_addend = target_addend - 1;
                        encoded_addend =
                            target_addend - static_cast<int32_t>(cur_offset_ + 1);
                    }
                    if (is_external_target(target, syms_))
                        emit_.refer_symbol(target.name);
                    emit_.emit_reloc(target.name,
                                     bfd::reloc_type::z80_pc8,
                                     target.sym_relative,
                                     reloc_addend);
                    emit_.emit_byte(static_cast<uint8_t>(encoded_addend & 0xFF));
                } else {
                    // cur_offset_ is after the opcode byte but before the offset.
                    int32_t off = v ? static_cast<int32_t>(*v)
                                      - static_cast<int32_t>(cur_offset_ + 1)
                                    : 0;
                    if (off < -128 || off > 127)
                        throw codegen_error(src_file_, line,
                            "relative jump out of range");
                    emit_.emit_byte(static_cast<uint8_t>(off));
                }
            }
            ++cur_offset_;
            note_section_offset();
        }

        // -----------------------------------------------------------------------
        // Instruction size helpers (for pass 1)
        // -----------------------------------------------------------------------

        bool is_ix_iy(const operand& op)
        {
            return op.kind == operand_kind::ind_ix_off
                || op.kind == operand_kind::ind_iy_off
                || (op.kind == operand_kind::reg
                    && (op.reg_name == "IX" || op.reg_name == "IY"
                        || op.reg_name == "IXH" || op.reg_name == "IXL"
                        || op.reg_name == "IYH" || op.reg_name == "IYL"));
        }

        // -----------------------------------------------------------------------
        // Instruction emitters
        // -----------------------------------------------------------------------

        void do_ld(const stmt& s, int line)
        {
            if (s.operands.size() != 2)
                throw codegen_error(src_file_, line, "LD requires 2 operands");

            const operand& dst = s.operands[0];
            const operand& src = s.operands[1];

            // LD r, r'
            if (dst.kind == operand_kind::reg && src.kind == operand_kind::reg) {
                int d = reg8(dst.reg_name);
                int s8 = reg8(src.reg_name);
                uint8_t dp = 0, sp = 0;
                int dh = -1, sh = -1;
                const bool dst_half = index_half(dst.reg_name, dp, dh);
                const bool src_half = index_half(src.reg_name, sp, sh);
                if ((dst_half || src_half) &&
                    (!dst_half || !src_half || dp == sp) &&
                    (dst_half || (d >= 0 && d != 4 && d != 5)) &&
                    (src_half || (s8 >= 0 && s8 != 4 && s8 != 5))) {
                    emit_byte_val(dst_half ? dp : sp);
                    emit_byte_val(static_cast<uint8_t>(
                        0x40 | ((dst_half ? dh : d) << 3) |
                        (src_half ? sh : s8)));
                    return;
                }
                if (d >= 0 && s8 >= 0) {
                    emit_byte_val(static_cast<uint8_t>(0x40 | (d << 3) | s8));
                    return;
                }
                // LD dd, ss (16-bit register to register — no direct opcode)
                // LD SP, HL
                if (dst.reg_name == "SP" && src.reg_name == "HL") {
                    emit_byte_val(0xF9); return;
                }
                if (dst.reg_name == "SP" && src.reg_name == "IX") {
                    emit_byte_val(0xDD); emit_byte_val(0xF9); return;
                }
                if (dst.reg_name == "SP" && src.reg_name == "IY") {
                    emit_byte_val(0xFD); emit_byte_val(0xF9); return;
                }
                // LD I, A / LD R, A / LD A, I / LD A, R
                if (dst.reg_name == "I" && src.reg_name == "A") {
                    emit_byte_val(0xED); emit_byte_val(0x47); return;
                }
                if (dst.reg_name == "R" && src.reg_name == "A") {
                    emit_byte_val(0xED); emit_byte_val(0x4F); return;
                }
                if (dst.reg_name == "A" && src.reg_name == "I") {
                    emit_byte_val(0xED); emit_byte_val(0x57); return;
                }
                if (dst.reg_name == "A" && src.reg_name == "R") {
                    emit_byte_val(0xED); emit_byte_val(0x5F); return;
                }
            }

            // LD r, n
            if (dst.kind == operand_kind::reg && src.kind == operand_kind::imm) {
                int d = reg8(dst.reg_name);
                uint8_t prefix = 0;
                int half = -1;
                if (index_half(dst.reg_name, prefix, half)) {
                    emit_byte_val(prefix);
                    emit_byte_val(static_cast<uint8_t>(0x06 | (half << 3)));
                    emit_byte_expr(*src.value, line);
                    return;
                }
                if (d >= 0) {
                    emit_byte_val(static_cast<uint8_t>(0x06 | (d << 3)));
                    emit_byte_expr(*src.value, line);
                    return;
                }
                if (dst.reg_name == "BC" || dst.reg_name == "DE"
                    || dst.reg_name == "HL" || dst.reg_name == "SP") {
                    int rr = reg16_sp(dst.reg_name);
                    emit_byte_val(static_cast<uint8_t>(0x01 | (rr << 4)));
                    emit_word_expr(*src.value, line);
                    return;
                }
                if (dst.reg_name == "IX") {
                    emit_byte_val(0xDD);
                    emit_byte_val(0x21);
                    emit_word_expr(*src.value, line);
                    return;
                }
                if (dst.reg_name == "IY") {
                    emit_byte_val(0xFD);
                    emit_byte_val(0x21);
                    emit_word_expr(*src.value, line);
                    return;
                }
            }

            // LD r, (HL)
            if (dst.kind == operand_kind::reg
                && src.kind == operand_kind::ind_reg
                && src.reg_name == "HL") {
                int d = reg8(dst.reg_name);
                if (d >= 0) {
                    emit_byte_val(static_cast<uint8_t>(0x46 | (d << 3)));
                    return;
                }
            }

            // LD r, (IX+d) / LD r, (IY+d)
            if (dst.kind == operand_kind::reg
                && (src.kind == operand_kind::ind_ix_off
                    || src.kind == operand_kind::ind_iy_off)) {
                int d = reg8(dst.reg_name);
                if (d >= 0) {
                    uint8_t pfx = (src.kind == operand_kind::ind_ix_off) ? 0xDD : 0xFD;
                    emit_byte_val(pfx);
                    emit_byte_val(static_cast<uint8_t>(0x46 | (d << 3)));
                    emit_byte_expr(*src.value, line);
                    return;
                }
            }

            // LD (HL), r
            if (dst.kind == operand_kind::ind_reg
                && dst.reg_name == "HL"
                && src.kind == operand_kind::reg) {
                int s8 = reg8(src.reg_name);
                if (s8 >= 0) {
                    emit_byte_val(static_cast<uint8_t>(0x70 | s8));
                    return;
                }
            }

            // LD (IX+d), r / LD (IY+d), r
            if ((dst.kind == operand_kind::ind_ix_off
                 || dst.kind == operand_kind::ind_iy_off)
                && src.kind == operand_kind::reg) {
                int s8 = reg8(src.reg_name);
                if (s8 >= 0) {
                    uint8_t pfx = (dst.kind == operand_kind::ind_ix_off) ? 0xDD : 0xFD;
                    emit_byte_val(pfx);
                    emit_byte_val(static_cast<uint8_t>(0x70 | s8));
                    emit_byte_expr(*dst.value, line);
                    return;
                }
            }

            // LD (HL), n
            if (dst.kind == operand_kind::ind_reg
                && dst.reg_name == "HL"
                && src.kind == operand_kind::imm) {
                emit_byte_val(0x36);
                emit_byte_expr(*src.value, line);
                return;
            }

            // LD (IX+d), n / LD (IY+d), n
            if ((dst.kind == operand_kind::ind_ix_off
                 || dst.kind == operand_kind::ind_iy_off)
                && src.kind == operand_kind::imm) {
                uint8_t pfx = (dst.kind == operand_kind::ind_ix_off) ? 0xDD : 0xFD;
                emit_byte_val(pfx); emit_byte_val(0x36);
                emit_byte_expr(*dst.value, line);
                emit_byte_expr(*src.value, line);
                return;
            }

            // LD A, (BC) / LD A, (DE)
            if (dst.kind == operand_kind::reg && dst.reg_name == "A"
                && src.kind == operand_kind::ind_reg) {
                if (src.reg_name == "BC") { emit_byte_val(0x0A); return; }
                if (src.reg_name == "DE") { emit_byte_val(0x1A); return; }
            }

            // LD (BC), A / LD (DE), A
            if (dst.kind == operand_kind::ind_reg
                && src.kind == operand_kind::reg && src.reg_name == "A") {
                if (dst.reg_name == "BC") { emit_byte_val(0x02); return; }
                if (dst.reg_name == "DE") { emit_byte_val(0x12); return; }
            }

            // LD A, (nn)
            if (dst.kind == operand_kind::reg && dst.reg_name == "A"
                && src.kind == operand_kind::ind_expr) {
                emit_byte_val(0x3A);
                emit_word_expr(*src.value, line);
                return;
            }

            // LD (nn), A
            if (dst.kind == operand_kind::ind_expr
                && src.kind == operand_kind::reg && src.reg_name == "A") {
                emit_byte_val(0x32);
                emit_word_expr(*dst.value, line);
                return;
            }

            // LD HL, (nn)
            if (dst.kind == operand_kind::reg && dst.reg_name == "HL"
                && src.kind == operand_kind::ind_expr) {
                emit_byte_val(0x2A);
                emit_word_expr(*src.value, line);
                return;
            }

            // LD (nn), HL
            if (dst.kind == operand_kind::ind_expr
                && src.kind == operand_kind::reg && src.reg_name == "HL") {
                emit_byte_val(0x22);
                emit_word_expr(*dst.value, line);
                return;
            }

            // LD dd, (nn) — ED prefix
            if (dst.kind == operand_kind::reg
                && src.kind == operand_kind::ind_expr) {
                int rr = reg16_sp(dst.reg_name);
                if (rr >= 0 && dst.reg_name != "HL") {
                    emit_byte_val(0xED);
                    emit_byte_val(static_cast<uint8_t>(0x4B | (rr << 4)));
                    emit_word_expr(*src.value, line);
                    return;
                }
                if (dst.reg_name == "IX") {
                    emit_byte_val(0xDD); emit_byte_val(0x2A);
                    emit_word_expr(*src.value, line); return;
                }
                if (dst.reg_name == "IY") {
                    emit_byte_val(0xFD); emit_byte_val(0x2A);
                    emit_word_expr(*src.value, line); return;
                }
            }

            // LD (nn), dd — ED prefix
            if (dst.kind == operand_kind::ind_expr
                && src.kind == operand_kind::reg) {
                int rr = reg16_sp(src.reg_name);
                if (rr >= 0 && src.reg_name != "HL") {
                    emit_byte_val(0xED);
                    emit_byte_val(static_cast<uint8_t>(0x43 | (rr << 4)));
                    emit_word_expr(*dst.value, line);
                    return;
                }
                if (src.reg_name == "IX") {
                    emit_byte_val(0xDD); emit_byte_val(0x22);
                    emit_word_expr(*dst.value, line); return;
                }
                if (src.reg_name == "IY") {
                    emit_byte_val(0xFD); emit_byte_val(0x22);
                    emit_word_expr(*dst.value, line); return;
                }
            }

            throw codegen_error(src_file_, line, "unrecognised LD form");
        }

        void do_alu(const stmt& s, int line)
        {
            // ALU op codes: ADD=0 ADC=1 SUB=2 SBC=3 AND=4 XOR=5 OR=6 CP=7
            static const char* NAMES[] = {
                "ADD","ADC","SUB","SBC","AND","XOR","OR","CP"
            };
            int op_idx = -1;
            for (int i = 0; i < 8; ++i)
                if (s.mnemonic == NAMES[i]) { op_idx = i; break; }
            if (op_idx < 0) return;

            bool has_a = (s.operands.size() == 2
                          && s.operands[0].kind == operand_kind::reg
                          && s.operands[0].reg_name == "A");

            const operand& src = has_a ? s.operands[1] : s.operands[0];

            // ADD HL, ss / ADC HL, ss / SBC HL, ss
            if ((s.mnemonic == "ADD" || s.mnemonic == "ADC"
                 || s.mnemonic == "SBC") && s.operands.size() == 2) {
                const operand& d = s.operands[0];
                const operand& r = s.operands[1];
                if (d.kind == operand_kind::reg && r.kind == operand_kind::reg) {
                    int rr = reg16_sp(r.reg_name);
                    if (d.reg_name == "HL" && rr >= 0) {
                        if (s.mnemonic == "ADD") {
                            emit_byte_val(static_cast<uint8_t>(0x09 | (rr << 4)));
                        } else {
                            uint8_t ed_op = (s.mnemonic == "ADC") ? 0x4A : 0x42;
                            emit_byte_val(0xED);
                            emit_byte_val(static_cast<uint8_t>(ed_op | (rr << 4)));
                        }
                        return;
                    }
                    if (d.reg_name == "IX") {
                        int rr2 = reg16_sp(r.reg_name);
                        if (rr2 < 0 && r.reg_name == "IX") rr2 = 2; // IX self-add = HL slot
                        if (rr2 >= 0 && s.mnemonic == "ADD") {
                            emit_byte_val(0xDD);
                            emit_byte_val(static_cast<uint8_t>(0x09 | (rr2 << 4)));
                            return;
                        }
                    }
                    if (d.reg_name == "IY") {
                        int rr2 = reg16_sp(r.reg_name);
                        if (rr2 < 0 && r.reg_name == "IY") rr2 = 2; // IY self-add = HL slot
                        if (rr2 >= 0 && s.mnemonic == "ADD") {
                            emit_byte_val(0xFD);
                            emit_byte_val(static_cast<uint8_t>(0x09 | (rr2 << 4)));
                            return;
                        }
                    }
                }
            }

            uint8_t base = static_cast<uint8_t>(0x80 | (op_idx << 3));
            uint8_t imm_op = static_cast<uint8_t>(0xC6 | (op_idx << 3));

            if (src.kind == operand_kind::reg) {
                int s8 = reg8(src.reg_name);
                uint8_t prefix = 0;
                int half = -1;
                if (index_half(src.reg_name, prefix, half)) {
                    emit_byte_val(prefix);
                    emit_byte_val(static_cast<uint8_t>(base | half));
                    return;
                }
                if (s8 >= 0) { emit_byte_val(base | s8); return; }
            }
            if (src.kind == operand_kind::ind_reg && src.reg_name == "HL") {
                emit_byte_val(base | 6); return;
            }
            if (src.kind == operand_kind::ind_ix_off
                || src.kind == operand_kind::ind_iy_off) {
                uint8_t pfx = (src.kind == operand_kind::ind_ix_off) ? 0xDD : 0xFD;
                emit_byte_val(pfx); emit_byte_val(base | 6);
                emit_byte_expr(*src.value, line); return;
            }
            if (src.kind == operand_kind::imm) {
                emit_byte_val(imm_op);
                emit_byte_expr(*src.value, line); return;
            }
            throw codegen_error(src_file_, line,
                "unrecognised ALU form: " + s.mnemonic);
        }

        void do_inc_dec(const stmt& s, int line)
        {
            bool is_inc = (s.mnemonic == "INC");
            if (s.operands.size() != 1)
                throw codegen_error(src_file_, line,
                    s.mnemonic + " requires 1 operand");
            const operand& op = s.operands[0];
            if (op.kind == operand_kind::reg) {
                int r8 = reg8(op.reg_name);
                if (r8 >= 0) {
                    emit_byte_val(static_cast<uint8_t>(
                        is_inc ? (0x04 | (r8 << 3)) : (0x05 | (r8 << 3))));
                    return;
                }
                int r16 = reg16_sp(op.reg_name);
                if (r16 >= 0) {
                    emit_byte_val(static_cast<uint8_t>(
                        is_inc ? (0x03 | (r16 << 4)) : (0x0B | (r16 << 4))));
                    return;
                }
                if (op.reg_name == "IX") {
                    emit_byte_val(0xDD);
                    emit_byte_val(is_inc ? 0x23 : 0x2B);
                    return;
                }
                if (op.reg_name == "IY") {
                    emit_byte_val(0xFD);
                    emit_byte_val(is_inc ? 0x23 : 0x2B);
                    return;
                }
            }
            if (op.kind == operand_kind::ind_reg && op.reg_name == "HL") {
                emit_byte_val(is_inc ? 0x34 : 0x35); return;
            }
            if (op.kind == operand_kind::ind_ix_off
                || op.kind == operand_kind::ind_iy_off) {
                uint8_t pfx = (op.kind == operand_kind::ind_ix_off) ? 0xDD : 0xFD;
                emit_byte_val(pfx);
                emit_byte_val(is_inc ? 0x34 : 0x35);
                emit_byte_expr(*op.value, line);
                return;
            }
            throw codegen_error(src_file_, line,
                "unrecognised " + s.mnemonic + " form");
        }

        void do_rotate(const stmt& s, int line)
        {
            // RLCA RRCA RLA RRA — single byte, no CB prefix
            if (s.mnemonic == "RLCA") { emit_byte_val(0x07); return; }
            if (s.mnemonic == "RRCA") { emit_byte_val(0x0F); return; }
            if (s.mnemonic == "RLA")  { emit_byte_val(0x17); return; }
            if (s.mnemonic == "RRA")  { emit_byte_val(0x1F); return; }

            // CB-prefix rotates: RLC RRC RL RR SLA SRA SRL SLL
            static const char* ROT_NAMES[] = {
                "RLC","RRC","RL","RR","SLA","SRA","SLL","SRL"
            };
            int rot_idx = -1;
            for (int i = 0; i < 8; ++i)
                if (s.mnemonic == ROT_NAMES[i]) { rot_idx = i; break; }
            if (rot_idx < 0) return;

            if (s.operands.size() != 1)
                throw codegen_error(src_file_, line,
                    s.mnemonic + " requires 1 operand");
            const operand& op = s.operands[0];

            uint8_t cb_base = static_cast<uint8_t>(rot_idx << 3);
            if (op.kind == operand_kind::reg) {
                int r8 = reg8(op.reg_name);
                if (r8 >= 0) {
                    emit_byte_val(0xCB); emit_byte_val(cb_base | r8); return;
                }
            }
            if (op.kind == operand_kind::ind_reg && op.reg_name == "HL") {
                emit_byte_val(0xCB); emit_byte_val(cb_base | 6); return;
            }
            if (op.kind == operand_kind::ind_ix_off
                || op.kind == operand_kind::ind_iy_off) {
                uint8_t pfx = (op.kind == operand_kind::ind_ix_off) ? 0xDD : 0xFD;
                emit_byte_val(pfx); emit_byte_val(0xCB);
                emit_byte_expr(*op.value, line);
                emit_byte_val(cb_base | 6);
                return;
            }
            throw codegen_error(src_file_, line,
                "unrecognised rotate form: " + s.mnemonic);
        }

        void do_bit(const stmt& s, int line)
        {
            static const char* BIT_NAMES[] = {"BIT","SET","RES"};
            int bit_idx = -1;
            for (int i = 0; i < 3; ++i)
                if (s.mnemonic == BIT_NAMES[i]) { bit_idx = i; break; }
            if (bit_idx < 0) return;

            if (s.operands.size() != 2)
                throw codegen_error(src_file_, line,
                    s.mnemonic + " requires 2 operands");

            auto bit_v = eval_expr(*s.operands[0].value, syms_, cur_offset_);
            int bit_num = bit_v ? static_cast<int>(*bit_v & 7) : 0;

            // CB base: BIT=0x40, RES=0x80, SET=0xC0
            uint8_t cb_base = static_cast<uint8_t>(
                (bit_idx == 0 ? 0x40 : (bit_idx == 2 ? 0x80 : 0xC0))
                | (bit_num << 3));

            const operand& op = s.operands[1];
            if (op.kind == operand_kind::reg) {
                int r8 = reg8(op.reg_name);
                if (r8 >= 0) {
                    emit_byte_val(0xCB); emit_byte_val(cb_base | r8); return;
                }
            }
            if (op.kind == operand_kind::ind_reg && op.reg_name == "HL") {
                emit_byte_val(0xCB); emit_byte_val(cb_base | 6); return;
            }
            if (op.kind == operand_kind::ind_ix_off
                || op.kind == operand_kind::ind_iy_off) {
                uint8_t pfx = (op.kind == operand_kind::ind_ix_off) ? 0xDD : 0xFD;
                emit_byte_val(pfx); emit_byte_val(0xCB);
                emit_byte_expr(*op.value, line);
                emit_byte_val(cb_base | 6);
                return;
            }
            throw codegen_error(src_file_, line,
                "unrecognised " + s.mnemonic + " form");
        }

        void do_jump(const stmt& s, int line)
        {
            if (s.mnemonic == "JP") {
                if (s.operands.empty())
                    throw codegen_error(src_file_, line, "JP requires operand");
                const operand& op0 = s.operands[0];
                // JP (HL) / JP (IX) / JP (IY)
                if (op0.kind == operand_kind::ind_reg
                    || op0.kind == operand_kind::ind_ix_off
                    || op0.kind == operand_kind::ind_iy_off) {
                    if (op0.reg_name == "HL") { emit_byte_val(0xE9); return; }
                    if (op0.reg_name == "IX") { emit_byte_val(0xDD); emit_byte_val(0xE9); return; }
                    if (op0.reg_name == "IY") { emit_byte_val(0xFD); emit_byte_val(0xE9); return; }
                }
                if (op0.kind == operand_kind::reg) {
                    if (op0.reg_name == "HL") { emit_byte_val(0xE9); return; }
                    if (op0.reg_name == "IX") { emit_byte_val(0xDD); emit_byte_val(0xE9); return; }
                    if (op0.reg_name == "IY") { emit_byte_val(0xFD); emit_byte_val(0xE9); return; }
                }
                // JP cc, nn ("C" register is ambiguous — also carry condition)
                if (s.operands.size() == 2) {
                    std::string cc_name;
                    if (op0.kind == operand_kind::cond)
                        cc_name = op0.cond_name;
                    else if (op0.kind == operand_kind::reg && op0.reg_name == "C")
                        cc_name = "C";
                    if (!cc_name.empty()) {
                        int cc = cond_code(cc_name);
                        if (cc < 0)
                            throw codegen_error(src_file_, line, "bad condition");
                        emit_byte_val(static_cast<uint8_t>(0xC2 | (cc << 3)));
                        emit_word_expr(*s.operands[1].value, line);
                        return;
                    }
                }
                // JP nn
                if (s.operands.size() == 1 && op0.kind == operand_kind::imm) {
                    emit_byte_val(0xC3);
                    emit_word_expr(*op0.value, line);
                    return;
                }
                throw codegen_error(src_file_, line, "unrecognised JP form");
            }

            if (s.mnemonic == "JR") {
                if (s.operands.size() == 2) {
                    const operand& op0 = s.operands[0];
                    // "C" is ambiguous: parsed as reg but used as carry condition.
                    std::string cc_name;
                    if (op0.kind == operand_kind::cond)
                        cc_name = op0.cond_name;
                    else if (op0.kind == operand_kind::reg && op0.reg_name == "C")
                        cc_name = "C";
                    if (!cc_name.empty()) {
                        int cc = jr_cond(cc_name);
                        if (cc < 0)
                            throw codegen_error(src_file_, line,
                                "condition not valid for JR");
                        emit_byte_val(static_cast<uint8_t>(0x20 | (cc << 3)));
                        emit_pcrel8(*s.operands[1].value, line);
                        return;
                    }
                }
                if (s.operands.size() == 1
                    && s.operands[0].kind == operand_kind::imm) {
                    emit_byte_val(0x18);
                    emit_pcrel8(*s.operands[0].value, line);
                    return;
                }
                throw codegen_error(src_file_, line, "unrecognised JR form");
            }

            if (s.mnemonic == "DJNZ") {
                if (s.operands.size() != 1)
                    throw codegen_error(src_file_, line, "DJNZ requires 1 operand");
                emit_byte_val(0x10);
                emit_pcrel8(*s.operands[0].value, line);
                return;
            }

            if (s.mnemonic == "CALL") {
                if (s.operands.size() == 2
                    && s.operands[0].kind == operand_kind::cond) {
                    int cc = cond_code(s.operands[0].cond_name);
                    if (cc < 0)
                        throw codegen_error(src_file_, line, "bad condition");
                    emit_byte_val(static_cast<uint8_t>(0xC4 | (cc << 3)));
                    emit_word_expr(*s.operands[1].value, line);
                    return;
                }
                if (s.operands.size() == 1
                    && s.operands[0].kind == operand_kind::imm) {
                    emit_byte_val(0xCD);
                    emit_word_expr(*s.operands[0].value, line);
                    return;
                }
                throw codegen_error(src_file_, line, "unrecognised CALL form");
            }

            if (s.mnemonic == "RET") {
                if (s.operands.empty()) { emit_byte_val(0xC9); return; }
                std::string cc_name;
                if (s.operands[0].kind == operand_kind::cond)
                    cc_name = s.operands[0].cond_name;
                else if (s.operands[0].kind == operand_kind::reg
                         && s.operands[0].reg_name == "C")
                    cc_name = "C";
                if (!cc_name.empty()) {
                    int cc = cond_code(cc_name);
                    if (cc >= 0) {
                        emit_byte_val(static_cast<uint8_t>(0xC0 | (cc << 3)));
                        return;
                    }
                }
                throw codegen_error(src_file_, line, "unrecognised RET form");
            }

            if (s.mnemonic == "RETI") { emit_byte_val(0xED); emit_byte_val(0x4D); return; }
            if (s.mnemonic == "RETN") { emit_byte_val(0xED); emit_byte_val(0x45); return; }

            if (s.mnemonic == "RST") {
                if (s.operands.size() != 1)
                    throw codegen_error(src_file_, line, "RST requires 1 operand");
                auto v = eval_expr(*s.operands[0].value, syms_, cur_offset_);
                uint8_t p = v ? static_cast<uint8_t>(*v & 0x38) : 0;
                emit_byte_val(static_cast<uint8_t>(0xC7 | p));
                return;
            }
        }

        void do_push_pop(const stmt& s, int line)
        {
            bool is_push = (s.mnemonic == "PUSH");
            if (s.operands.size() != 1)
                throw codegen_error(src_file_, line,
                    s.mnemonic + " requires 1 operand");
            const operand& op = s.operands[0];
            if (op.kind == operand_kind::reg) {
                int qq = reg16_af(op.reg_name);
                if (qq >= 0) {
                    emit_byte_val(static_cast<uint8_t>(
                        is_push ? (0xC5 | (qq << 4)) : (0xC1 | (qq << 4))));
                    return;
                }
                if (op.reg_name == "IX") {
                    emit_byte_val(0xDD); emit_byte_val(is_push ? 0xE5 : 0xE1); return;
                }
                if (op.reg_name == "IY") {
                    emit_byte_val(0xFD); emit_byte_val(is_push ? 0xE5 : 0xE1); return;
                }
            }
            throw codegen_error(src_file_, line,
                "unrecognised " + s.mnemonic + " operand");
        }

        void do_exchange(const stmt& s, int line)
        {
            if (s.operands.size() != 2)
                throw codegen_error(src_file_, line, "EX requires 2 operands");
            const operand& a = s.operands[0];
            const operand& b = s.operands[1];
            if (a.kind == operand_kind::reg && b.kind == operand_kind::reg) {
                if (a.reg_name == "DE" && b.reg_name == "HL") { emit_byte_val(0xEB); return; }
                if (a.reg_name == "AF" && b.reg_name == "AF'") { emit_byte_val(0x08); return; }
            }
            if (a.kind == operand_kind::ind_reg && a.reg_name == "SP") {
                if (b.kind == operand_kind::reg) {
                    if (b.reg_name == "HL") { emit_byte_val(0xE3); return; }
                    if (b.reg_name == "IX") { emit_byte_val(0xDD); emit_byte_val(0xE3); return; }
                    if (b.reg_name == "IY") { emit_byte_val(0xFD); emit_byte_val(0xE3); return; }
                }
            }
            throw codegen_error(src_file_, line, "unrecognised EX form");
        }

        void do_io(const stmt& s, int line)
        {
            if (s.mnemonic == "IN") {
                if (s.operands.size() == 2) {
                    const operand& dst = s.operands[0];
                    const operand& src = s.operands[1];
                    if (dst.kind == operand_kind::reg && dst.reg_name == "A"
                        && src.kind == operand_kind::ind_expr) {
                        emit_byte_val(0xDB);
                        emit_byte_expr(*src.value, line);
                        return;
                    }
                    if (dst.kind == operand_kind::reg
                        && src.kind == operand_kind::ind_reg
                        && src.reg_name == "C") {
                        int r = reg8(dst.reg_name);
                        if (r >= 0) {
                            emit_byte_val(0xED);
                            emit_byte_val(static_cast<uint8_t>(0x40 | (r << 3)));
                            return;
                        }
                    }
                }
                throw codegen_error(src_file_, line, "unrecognised IN form");
            }
            if (s.mnemonic == "OUT") {
                if (s.operands.size() == 2) {
                    const operand& dst = s.operands[0];
                    const operand& src = s.operands[1];
                    if (dst.kind == operand_kind::ind_expr
                        && src.kind == operand_kind::reg && src.reg_name == "A") {
                        emit_byte_val(0xD3);
                        emit_byte_expr(*dst.value, line);
                        return;
                    }
                    if (dst.kind == operand_kind::ind_reg && dst.reg_name == "C"
                        && src.kind == operand_kind::reg) {
                        int r = reg8(src.reg_name);
                        if (r >= 0) {
                            emit_byte_val(0xED);
                            emit_byte_val(static_cast<uint8_t>(0x41 | (r << 3)));
                            return;
                        }
                    }
                }
                throw codegen_error(src_file_, line, "unrecognised OUT form");
            }
        }

        // Simple no-operand instructions.
        void do_simple(const stmt& s, int)
        {
            struct { const char* mn; uint8_t b1, b2; } table[] = {
                {"NOP",  0x00, 0},
                {"HALT", 0x76, 0},
                {"DI",   0xF3, 0},
                {"EI",   0xFB, 0},
                {"EXX",  0xD9, 0},
                {"DAA",  0x27, 0},
                {"CPL",  0x2F, 0},
                {"SCF",  0x37, 0},
                {"CCF",  0x3F, 0},
                {"LDI",  0xED, 0xA0},
                {"LDD",  0xED, 0xA8},
                {"LDIR", 0xED, 0xB0},
                {"LDDR", 0xED, 0xB8},
                {"CPI",  0xED, 0xA1},
                {"CPD",  0xED, 0xA9},
                {"CPIR", 0xED, 0xB1},
                {"CPDR", 0xED, 0xB9},
                {"INI",  0xED, 0xA2},
                {"IND",  0xED, 0xAA},
                {"INIR", 0xED, 0xB2},
                {"INDR", 0xED, 0xBA},
                {"OUTI", 0xED, 0xA3},
                {"OUTD", 0xED, 0xAB},
                {"OTIR", 0xED, 0xB3},
                {"OTDR", 0xED, 0xBB},
                {"NEG",  0xED, 0x44},
                {"RETI", 0xED, 0x4D},
                {"RETN", 0xED, 0x45},
                {"RLD",  0xED, 0x6F},
                {"RRD",  0xED, 0x67},
            };
            for (auto& e : table) {
                if (s.mnemonic == e.mn) {
                    emit_byte_val(e.b1);
                    if (e.b2) emit_byte_val(e.b2);
                    return;
                }
            }
        }

        void do_im(const stmt& s, int line)
        {
            if (s.operands.size() != 1)
                throw codegen_error(src_file_, line, "IM requires 1 operand");
            auto v = eval_expr(*s.operands[0].value, syms_, cur_offset_);
            uint8_t mode = v ? static_cast<uint8_t>(*v) : 0;
            emit_byte_val(0xED);
            switch (mode) {
                case 0: emit_byte_val(0x46); break;
                case 1: emit_byte_val(0x56); break;
                case 2: emit_byte_val(0x5E); break;
                default: throw codegen_error(src_file_, line, "IM mode must be 0, 1, or 2");
            }
        }

        // -----------------------------------------------------------------------
        // Instruction size calculator (pass 1)
        // -----------------------------------------------------------------------

        uint32_t instr_size(const stmt& s)
        {
            const std::string& mn = s.mnemonic;

            // Simple 1-byte or 2-byte instructions.
            static const char* ONE[] = {
                "NOP","HALT","DI","EI","EXX","DAA","CPL","SCF","CCF",
                "RLCA","RRCA","RLA","RRA"
            };
            for (auto* x : ONE) if (mn == x) return 1;

            static const char* TWO_ED[] = {
                "LDI","LDD","LDIR","LDDR","CPI","CPD","CPIR","CPDR",
                "INI","IND","INIR","INDR","OUTI","OUTD","OTIR","OTDR",
                "NEG","RETI","RETN","RLD","RRD"
            };
            for (auto* x : TWO_ED) if (mn == x) return 2;

            if (mn == "EX" || mn == "EXX") return 1;

            if (mn == "PUSH" || mn == "POP") {
                if (!s.operands.empty()) {
                    const std::string& r = s.operands[0].reg_name;
                    if (r == "IX" || r == "IY") return 2;
                }
                return 1;
            }

            if (mn == "INC" || mn == "DEC") {
                if (!s.operands.empty()) {
                    const operand& op = s.operands[0];
                    if (op.kind == operand_kind::ind_ix_off
                        || op.kind == operand_kind::ind_iy_off) return 3;
                    if (op.kind == operand_kind::reg
                        && (op.reg_name == "IX" || op.reg_name == "IY")) return 2;
                }
                return 1;
            }

            if (mn == "LD") return ld_size(s);

            if (mn == "ADD" || mn == "ADC" || mn == "SUB" || mn == "SBC"
                || mn == "AND" || mn == "XOR" || mn == "OR" || mn == "CP")
                return alu_size(s);

            if (mn == "JP") {
                if (s.operands.size() == 1) {
                    if (s.operands[0].kind == operand_kind::reg
                        || s.operands[0].kind == operand_kind::ind_reg
                        || s.operands[0].kind == operand_kind::ind_ix_off
                        || s.operands[0].kind == operand_kind::ind_iy_off) {
                        const std::string& r = s.operands[0].reg_name;
                        if (r == "IX" || r == "IY") return 2;
                        return 1;
                    }
                }
                return 3;
            }
            if (mn == "JR" || mn == "DJNZ") return 2;
            if (mn == "CALL") return 3;
            if (mn == "RET") {
                if (s.operands.empty()) return 1;
                return 1;
            }
            if (mn == "RETI" || mn == "RETN") return 2;
            if (mn == "RST") return 1;
            if (mn == "IM") return 2;
            if (mn == "IN" || mn == "OUT") return 2;

            // CB prefix rotates.
            static const char* ROT[] = {"RLC","RRC","RL","RR","SLA","SRA","SRL","SLL"};
            for (auto* x : ROT) {
                if (mn == x) {
                    if (!s.operands.empty()
                        && (s.operands[0].kind == operand_kind::ind_ix_off
                            || s.operands[0].kind == operand_kind::ind_iy_off))
                        return 4;
                    return 2;
                }
            }

            static const char* BIT[] = {"BIT","SET","RES"};
            for (auto* x : BIT) {
                if (mn == x) {
                    if (s.operands.size() > 1
                        && (s.operands[1].kind == operand_kind::ind_ix_off
                            || s.operands[1].kind == operand_kind::ind_iy_off))
                        return 4;
                    return 2;
                }
            }

            return 1; // fallback
        }

        uint32_t ld_size(const stmt& s)
        {
            if (s.operands.size() != 2) return 1;
            const operand& dst = s.operands[0];
            const operand& src = s.operands[1];

            // Special SP variants.
            if (dst.reg_name == "SP" && (src.reg_name == "IX" || src.reg_name == "IY")) return 2;
            if (dst.reg_name == "SP" && src.reg_name == "HL") return 1;

            // I/R loads.
            if ((dst.reg_name == "I" || dst.reg_name == "R" ||
                 src.reg_name == "I" || src.reg_name == "R")) return 2;

            // r, r'
            if (dst.kind == operand_kind::reg && src.kind == operand_kind::reg) {
                uint8_t dp = 0, sp = 0;
                int dh = -1, sh = -1;
                if (index_half(dst.reg_name, dp, dh) ||
                    index_half(src.reg_name, sp, sh)) return 2;
                if (reg8(dst.reg_name) >= 0 && reg8(src.reg_name) >= 0) return 1;
            }
            // r, n
            if (dst.kind == operand_kind::reg && src.kind == operand_kind::imm) {
                uint8_t prefix = 0;
                int half = -1;
                if (index_half(dst.reg_name, prefix, half)) return 3;
                if (reg8(dst.reg_name) >= 0) return 2;
                if (dst.reg_name == "IX" || dst.reg_name == "IY") return 4;
                if (reg16_sp(dst.reg_name) >= 0) return 3;
            }
            // r, (HL)
            if (dst.kind == operand_kind::reg
                && src.kind == operand_kind::ind_reg && src.reg_name == "HL")
                return 1;
            // r, (IX+d) / r, (IY+d)
            if (dst.kind == operand_kind::reg
                && (src.kind == operand_kind::ind_ix_off
                    || src.kind == operand_kind::ind_iy_off)) return 3;
            // (HL), r
            if (dst.kind == operand_kind::ind_reg && dst.reg_name == "HL"
                && src.kind == operand_kind::reg && reg8(src.reg_name) >= 0) return 1;
            // (IX+d), r / (IY+d), r
            if ((dst.kind == operand_kind::ind_ix_off
                 || dst.kind == operand_kind::ind_iy_off)
                && src.kind == operand_kind::reg) return 3;
            // (HL), n
            if (dst.kind == operand_kind::ind_reg && dst.reg_name == "HL"
                && src.kind == operand_kind::imm) return 2;
            // (IX+d), n
            if ((dst.kind == operand_kind::ind_ix_off
                 || dst.kind == operand_kind::ind_iy_off)
                && src.kind == operand_kind::imm) return 4;
            // A, (BC) / A, (DE) / (BC), A / (DE), A
            if ((dst.kind == operand_kind::reg && dst.reg_name == "A"
                 && src.kind == operand_kind::ind_reg
                 && (src.reg_name == "BC" || src.reg_name == "DE")) ||
                (dst.kind == operand_kind::ind_reg
                 && (dst.reg_name == "BC" || dst.reg_name == "DE")
                 && src.kind == operand_kind::reg && src.reg_name == "A"))
                return 1;
            // A,(nn) / (nn),A / HL,(nn) / (nn),HL
            if (dst.kind == operand_kind::ind_expr || src.kind == operand_kind::ind_expr) {
                if ((dst.reg_name == "IX" || dst.reg_name == "IY"
                     || src.reg_name == "IX" || src.reg_name == "IY")) return 4;
                if ((dst.reg_name == "HL" || src.reg_name == "HL")) return 3;
                if (reg16_sp(dst.reg_name) >= 0
                    || reg16_sp(src.reg_name) >= 0) return 4;
                return 3;
            }
            return 1;
        }

        uint32_t alu_size(const stmt& s)
        {
            const operand* src = nullptr;
            if (s.operands.size() == 2) src = &s.operands[1];
            else if (s.operands.size() == 1) src = &s.operands[0];
            else return 1;

            // ADD HL,ss / ADC HL,ss / SBC HL,ss = 1 or 2 bytes
            if (s.operands.size() == 2
                && s.operands[0].kind == operand_kind::reg
                && s.operands[0].reg_name == "HL")
                return (s.mnemonic == "ADD") ? 1 : 2;
            if (s.operands.size() == 2
                && s.operands[0].kind == operand_kind::reg
                && (s.operands[0].reg_name == "IX" || s.operands[0].reg_name == "IY"))
                return 2;

            if (src->kind == operand_kind::reg) {
                uint8_t prefix = 0;
                int half = -1;
                return index_half(src->reg_name, prefix, half) ? 2 : 1;
            }
            if (src->kind == operand_kind::ind_reg && src->reg_name == "HL") return 1;
            if (src->kind == operand_kind::ind_ix_off
                || src->kind == operand_kind::ind_iy_off) return 3;
            if (src->kind == operand_kind::imm) return 2;
            return 1;
        }

        // -----------------------------------------------------------------------
        // Directive handler
        // -----------------------------------------------------------------------

        void process_directive(const stmt& s)
        {
            const std::string& dn = s.directive_name;

            if (dn == "area" || dn == "section") {
                std::string sec_name = s.string_arg;
                if (sec_name.empty()) sec_name = "_CODE";
                switch_section(sec_name);
                return;
            }

            if (dn == "globl" || dn == "global") {
                for (const auto& arg : s.args) {
                    if (arg->kind == expr_kind::symbol)
                        syms_[arg->name].global = true;
                }
                return;
            }

            if (dn == "extern" || dn == "external"
                || dn == "ref" || dn == "xref") {
                for (const auto& arg : s.args) {
                    if (arg->kind != expr_kind::symbol)
                        continue;
                    syms_[arg->name].global = true;
                    syms_[arg->name].external = true;
                    if (pass_ == 2)
                        emit_.refer_symbol(arg->name);
                }
                return;
            }

            if (dn == "org" || dn == "origin") {
                if (!s.args.empty()) {
                    auto v = eval_expr(*s.args[0], syms_, cur_offset_);
                    if (v) cur_offset_ = static_cast<uint32_t>(*v);
                }
                note_section_offset();
                return;
            }

            if (dn == "byte" || dn == "db") {
                for (char c : s.string_arg) {
                    emit_byte_val(static_cast<uint8_t>(c));
                }
                for (const auto& arg : s.args) {
                    emit_byte_expr(*arg, s.source_line);
                }
                return;
            }

            if (dn == "word" || dn == "dw" || dn == "2byte") {
                for (const auto& arg : s.args) {
                    emit_word_expr(*arg, s.source_line);
                }
                return;
            }

            if (dn == "ascii" || dn == "asciz" || dn == "string") {
                for (char c : s.string_arg)
                    emit_byte_val(static_cast<uint8_t>(c));
                return;
            }

            if (dn == "ds" || dn == "space") {
                uint32_t n = 0;
                if (!s.args.empty()) {
                    auto v = eval_expr(*s.args[0], syms_, cur_offset_);
                    if (v) n = static_cast<uint32_t>(*v);
                }
                emit_fill_or_space(n, directive_fill_byte(s), s.source_line);
                return;
            }

            if (dn == "align" || dn == "balign" || dn == "p2align") {
                const uint32_t n = directive_alignment_padding(s);
                emit_fill_or_space(n, directive_fill_byte(s), s.source_line);
                return;
            }

            // .dl expr — 32-bit little-endian word (SDCC extension)
            if (dn == "dl") {
                for (const auto &arg : s.args) {
                    auto v = eval_expr(*arg, syms_, cur_offset_);
                    if (pass_ == 2) {
                        ensure_section();
                        uint32_t val = v ? static_cast<uint32_t>(*v) : 0;
                        emit_.emit_word(static_cast<uint16_t>(val & 0xFFFF));
                        emit_.emit_word(static_cast<uint16_t>((val >> 16) & 0xFFFF));
                    }
                    cur_offset_ += 4;
                }
                note_section_offset();
                return;
            }

            // .blkw N — reserve N words (2N bytes) (SDCC extension)
            if (dn == "blkw") {
                uint32_t n = 0;
                if (!s.args.empty()) {
                    auto v = eval_expr(*s.args[0], syms_, cur_offset_);
                    if (v) n = static_cast<uint32_t>(*v);
                }
                if (pass_ == 2) {
                    ensure_section();
                    emit_.emit_space(n * 2, s.source_line);
                }
                cur_offset_ += n * 2;
                note_section_offset();
                return;
            }

            // .define symbol [= value] — absolute symbol definition (SDCC extension)
            if (dn == "define") {
                if (!s.string_arg.empty() && !s.args.empty()) {
                    auto v = eval_expr(*s.args[0], syms_, cur_offset_);
                    if (v) {
                        syms_[s.string_arg].value   = static_cast<uint32_t>(*v);
                        syms_[s.string_arg].defined = true;
                        if (pass_ == 2) {
                            emit_.define_symbol(s.string_arg,
                                                static_cast<uint32_t>(*v),
                                                "",
                                                syms_[s.string_arg].global);
                        }
                    }
                }
                return;
            }

            if (dn == "equ" || dn == "set") {
                if (!s.string_arg.empty() && !s.args.empty()) {
                    auto v = eval_expr(*s.args[0], syms_, cur_offset_);
                    if (v) {
                        syms_[s.string_arg].value   = static_cast<uint32_t>(*v);
                        syms_[s.string_arg].defined = true;
                        if (pass_ == 2) {
                            emit_.define_symbol(s.string_arg,
                                                static_cast<uint32_t>(*v),
                                                "",
                                                syms_[s.string_arg].global);
                        }
                    }
                }
                return;
            }
            if (dn == "type") {
                if (!s.string_arg.empty()) {
                    auto flags = symbol_type_flags(s.string_arg2);
                    syms_[s.string_arg].type_flags = flags;
                    if (pass_ == 2)
                        emit_.set_symbol_type(s.string_arg, flags);
                }
                return;
            }
            if (dn == "size") {
                if (!s.string_arg.empty() && !s.args.empty()) {
                    auto v = eval_expr(*s.args[0], syms_, cur_offset_);
                    if (v) {
                        syms_[s.string_arg].size = static_cast<uint64_t>(*v);
                        if (pass_ == 2)
                            emit_.set_symbol_size(s.string_arg,
                                                  static_cast<uint64_t>(*v));
                    }
                }
                return;
            }
            if (dn == "optsdcc") {
                if (pass_ == 2) {
                    if (auto cc = parse_optsdcc_cc(s.string_arg2))
                        emit_.set_default_calling_convention(*cc);
                }
                return;
            }
            // Other directives: module, file, include, conditionals — skip.
        }

        // -----------------------------------------------------------------------
        // Process one statement
        // -----------------------------------------------------------------------

        void process_stmt(const stmt& s)
        {
            if (s.kind == stmt_kind::comment)
                return;

            if (s.kind == stmt_kind::label) {
                if (pass_ == 1) {
                    syms_[s.label_name].section_name = cur_section_;
                    syms_[s.label_name].value        = cur_offset_;
                    syms_[s.label_name].defined      = true;
                    if (s.label_global)
                        syms_[s.label_name].global = true;
                } else {
                    ensure_section();
                    bool is_global = syms_.count(s.label_name)
                                  && syms_[s.label_name].global;
                    emit_.define_symbol(s.label_name, cur_offset_,
                                        cur_section_, is_global);
                    auto sit = syms_.find(s.label_name);
                    if (sit != syms_.end()) {
                        if (sit->second.type_flags != xbfd::symbol_flags::none)
                            emit_.set_symbol_type(s.label_name,
                                                  sit->second.type_flags);
                        if (sit->second.size != 0)
                            emit_.set_symbol_size(s.label_name,
                                                  sit->second.size);
                    }
                    emit_.mark_label(s.source_line);
                }
                return;
            }

            if (s.kind == stmt_kind::equ) {
                if (s.equ_global)
                    syms_[s.equ_name].global = true;
                auto v = eval_expr(*s.equ_value, syms_, cur_offset_);
                if (v) {
                    syms_[s.equ_name].value   = static_cast<uint32_t>(*v);
                    syms_[s.equ_name].defined = true;
                    if (pass_ == 2) {
                        emit_.define_symbol(s.equ_name,
                                            static_cast<uint32_t>(*v),
                                            "",
                                            syms_[s.equ_name].global);
                    }
                }
                return;
            }

            if (s.kind == stmt_kind::directive) {
                process_directive(s);
                return;
            }

            if (s.kind == stmt_kind::instruction) {
                const std::string& mn = s.mnemonic;
                int line = s.source_line;

                if (mn == "LD") { do_ld(s, line); return; }
                if (mn == "ADD" || mn == "ADC" || mn == "SUB" || mn == "SBC"
                    || mn == "AND" || mn == "XOR" || mn == "OR"  || mn == "CP")
                { do_alu(s, line); return; }
                if (mn == "INC" || mn == "DEC") { do_inc_dec(s, line); return; }
                if (mn == "RLCA" || mn == "RRCA" || mn == "RLA" || mn == "RRA"
                    || mn == "RLC" || mn == "RRC" || mn == "RL" || mn == "RR"
                    || mn == "SLA" || mn == "SRA" || mn == "SRL" || mn == "SLL")
                { do_rotate(s, line); return; }
                if (mn == "BIT" || mn == "SET" || mn == "RES")
                { do_bit(s, line); return; }
                if (mn == "JP" || mn == "JR" || mn == "DJNZ"
                    || mn == "CALL" || mn == "RET" || mn == "RETI"
                    || mn == "RETN" || mn == "RST")
                { do_jump(s, line); return; }
                if (mn == "PUSH" || mn == "POP") { do_push_pop(s, line); return; }
                if (mn == "EX")  { do_exchange(s, line); return; }
                if (mn == "IN" || mn == "OUT") { do_io(s, line); return; }
                if (mn == "IM") { do_im(s, line); return; }
                do_simple(s, line);
            }
        }

        // -----------------------------------------------------------------------
        // run — public entry point
        // -----------------------------------------------------------------------

        void run(const stmt_list& stmts, const std::string& module_name,
                 asm_mode mode, const std::vector<std::string>& defines)
        {
            install_predefines(defines);
            const std::vector<const stmt*> active_stmts =
                filter_conditionals(stmts);

            // Initialise emitter.
            emit_.begin_module(module_name);

            // Default section.
            std::string default_sec = (mode == asm_mode::gnu) ? ".text" : "_CODE";
            cur_section_ = default_sec;
            cur_offset_  = 0;
            section_offsets_.clear();
            section_offsets_[default_sec] = 0;

            // Pass 1: collect labels.
            pass_ = 1;
            cur_offset_ = 0;
            section_ready_ = false;
            for (const stmt* sp : active_stmts) {
                const stmt& s = *sp;
                if (s.kind == stmt_kind::comment)
                    continue;
                if (s.kind == stmt_kind::directive
                    && (s.directive_name == "area" || s.directive_name == "section")) {
                    cur_section_ = s.string_arg.empty() ? default_sec : s.string_arg;
                    cur_offset_  = section_offsets_[cur_section_];
                    continue;
                }
                if (s.kind == stmt_kind::label) {
                    syms_[s.label_name].value        = cur_offset_;
                    syms_[s.label_name].section_name = cur_section_;
                    syms_[s.label_name].defined      = true;
                    if (s.label_global)
                        syms_[s.label_name].global = true;
                    continue;
                }
                if (s.kind == stmt_kind::equ) {
                    if (s.equ_global)
                        syms_[s.equ_name].global = true;
                    auto v = eval_expr(*s.equ_value, syms_, cur_offset_);
                    if (v) {
                        syms_[s.equ_name].value   = static_cast<uint32_t>(*v);
                        syms_[s.equ_name].defined = true;
                    }
                    continue;
                }
                if (s.kind == stmt_kind::instruction) {
                    cur_offset_ += instr_size(s);
                    note_section_offset();
                    continue;
                }
                // Directives that affect offset.
                if (s.kind == stmt_kind::directive) {
                    const std::string& dn = s.directive_name;
                    if ((dn == "equ" || dn == "set")
                        && !s.string_arg.empty() && !s.args.empty()) {
                        auto v = eval_expr(*s.args[0], syms_, cur_offset_);
                        if (v) {
                            syms_[s.string_arg].value   = static_cast<uint32_t>(*v);
                            syms_[s.string_arg].defined = true;
                        }
                        continue;
                    }
                    if (dn == "define"
                        && !s.string_arg.empty() && !s.args.empty()) {
                        auto v = eval_expr(*s.args[0], syms_, cur_offset_);
                        if (v) {
                            syms_[s.string_arg].value   = static_cast<uint32_t>(*v);
                            syms_[s.string_arg].defined = true;
                        }
                        continue;
                    }
                    if (dn == "type" && !s.string_arg.empty()) {
                        syms_[s.string_arg].type_flags =
                            symbol_type_flags(s.string_arg2);
                        continue;
                    }
                    if (dn == "size"
                        && !s.string_arg.empty() && !s.args.empty()) {
                        auto v = eval_expr(*s.args[0], syms_, cur_offset_);
                        if (v)
                            syms_[s.string_arg].size = static_cast<uint64_t>(*v);
                        continue;
                    }
                    if (dn == "byte" || dn == "db")
                        cur_offset_ += static_cast<uint32_t>(s.string_arg.size()
                                                              + s.args.size());
                    else if (dn == "word" || dn == "dw" || dn == "2byte")
                        cur_offset_ += 2 * static_cast<uint32_t>(s.args.size());
                    else if (dn == "dl")
                        cur_offset_ += 4 * static_cast<uint32_t>(s.args.size());
                    else if (dn == "ascii" || dn == "asciz" || dn == "string")
                        cur_offset_ += static_cast<uint32_t>(s.string_arg.size());
                    else if (dn == "blkw") {
                        if (!s.args.empty()) {
                            auto v = eval_expr(*s.args[0], syms_, cur_offset_);
                            if (v) cur_offset_ += static_cast<uint32_t>(*v) * 2;
                        }
                    }
                    else if (dn == "ds" || dn == "space") {
                        if (!s.args.empty()) {
                            auto v = eval_expr(*s.args[0], syms_, cur_offset_);
                            if (v) cur_offset_ += static_cast<uint32_t>(*v);
                        }
                    }
                    else if (dn == "align" || dn == "balign" || dn == "p2align")
                        cur_offset_ += directive_alignment_padding(s);
                    note_section_offset();
                }
            }

            // Mark .globl symbols as referenced-but-possibly-external.
            for (const stmt* sp : active_stmts) {
                const stmt& s = *sp;
                if (s.kind == stmt_kind::comment)
                    continue;
                if (s.kind == stmt_kind::directive
                    && (s.directive_name == "globl" || s.directive_name == "global"
                        || s.directive_name == "extern"
                        || s.directive_name == "external"
                        || s.directive_name == "ref"
                        || s.directive_name == "xref")) {
                    for (const auto& arg : s.args) {
                        if (arg->kind == expr_kind::symbol)
                            syms_[arg->name].global = true;
                    }
                }
            }

            // Pass 2: emit code.
            pass_ = 2;
            cur_section_ = default_sec;
            cur_offset_  = 0;
            section_offsets_.clear();
            section_offsets_[default_sec] = 0;
            section_ready_ = false;
            emitted_sections_.clear();

            // Emit undefined globals in declaration order before codegen so
            // the REL symbol table matches SDCC's .globl-driven ordering.
            for (const stmt* sp : active_stmts) {
                const stmt& s = *sp;
                if (s.kind == stmt_kind::comment)
                    continue;
                if (s.kind != stmt_kind::directive)
                    continue;
                if (s.directive_name != "globl" && s.directive_name != "global")
                    continue;
                for (const auto& arg : s.args) {
                    if (arg->kind != expr_kind::symbol)
                        continue;
                    emit_.refer_symbol(arg->name);
                }
            }

            for (const stmt* sp : active_stmts)
                process_stmt(*sp);

            if (!emitted_sections_.count(default_sec)) {
                bfd::section_flags sf = classify_section_flags(default_sec);
                emit_.set_section(default_sec, sf);
                emitted_sections_[default_sec] = true;
            }

            emit_.end_module();
        }
    };

    // =========================================================================
    // Public entry point
    // =========================================================================

    void assemble(const stmt_list& stmts, emitter& emit,
                  const std::string& module_name,
                  const std::string& src_file,
                  asm_mode mode,
                  const std::vector<std::string>& defines)
    {
        codegen cg(emit, src_file);
        cg.run(stmts, module_name, mode, defines);
    }

} // namespace xas
