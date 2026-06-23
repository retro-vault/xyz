// macro.h
//
// xas macro preprocessor.  This stage runs on raw source lines *before*
// the lexer, expanding assembler macros and repeat blocks into plain
// source text which is then lexed and parsed normally.
//
// The two supported assembler families have very different macro syntaxes,
// so the engine is split into:
//
//   macro_processor   — dialect-agnostic engine: collects definitions,
//                       tracks nesting, expands calls and repeat blocks,
//                       enforces the recursion limit.
//
//   macro_dialect     — abstract strategy describing everything that is
//                       lexically specific to one assembler family.  Two
//                       concretions are provided:
//
//                         gnu_macro_dialect   — GNU as (gas) syntax
//                         sdas_macro_dialect  — SDCC sdasz80 / ASxxxx syntax
//
// Adding a third dialect means deriving one more macro_dialect and listing
// it in make_macro_dialect(); the engine needs no changes.
//
// The processor is a strict no-op when a source file contains no macro or
// repeat directives, so files that use no macros are byte-for-byte
// unaffected and keep their original line numbering.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#ifndef XAS_FRONTEND_MACRO_HPP
#define XAS_FRONTEND_MACRO_HPP

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <xas/cli.h>

namespace xas {

    // -------------------------------------------------------------------------
    // Source model
    // -------------------------------------------------------------------------

    // One physical source line, carrying origin information for diagnostics.
    struct src_line {
        std::string text;   // line content (no trailing newline)
        std::string file;   // origin file name
        int         line = 0;
    };

    using src_lines = std::vector<src_line>;

    // -------------------------------------------------------------------------
    // Macro definition model
    // -------------------------------------------------------------------------

    struct macro_param {
        std::string name;
        std::string default_val;
        bool        has_default = false;
        bool        required    = false;  // gas ':req'
        bool        vararg      = false;  // gas ':vararg'
        bool        autolocal   = false;  // sdas '?arg'
    };

    struct macro_def {
        std::string              name;
        std::vector<macro_param> params;
        src_lines                body;
    };

    using macro_binding = std::map<std::string, std::string>;

    // Mutable state threaded through one preprocessing run.
    struct expand_context {
        long global_count = 0;       // gas '\@'  — macros executed so far
        long plus_count   = 0;       // gas '\+'  — current per-block iteration
        long local_seq    = 10000;   // sdas auto-local symbol counter (n$ base)
        long cur_argc     = 0;       // sdas '.narg' — args in current expansion
    };

    // -------------------------------------------------------------------------
    // Dialect strategy
    // -------------------------------------------------------------------------

    class macro_dialect {
    public:
        enum class block { none, macro, rept, irp, irpc };

        virtual ~macro_dialect() = default;

        // Human-readable dialect name (diagnostics).
        virtual const char* name() const = 0;

        // Macro names: case sensitivity differs (gas: yes, sdas: no).
        virtual bool case_sensitive_names() const = 0;

        // Return the line with any trailing line-comment removed.
        virtual std::string strip_comment(const std::string& line) const = 0;

        // Classify the operator word (lowercased, leading '.' preserved).
        virtual block opener(const std::string& op) const = 0;
        virtual bool  closes(const std::string& op, block b) const = 0;
        virtual bool  is_exit(const std::string& op) const = 0; // .exitm / .mexit

        // Parse a '.macro' header line into name + parameters (body added later).
        virtual macro_def parse_macro_header(const src_line& line) const = 0;

        // Split an operand string into raw arguments, honoring this dialect's
        // grouping / quoting rules.  Used for macro calls and .irp lists.
        virtual std::vector<std::string>
            split_args(const std::string& operand) const = 0;

        // Build the parameter binding for a macro call.  May mutate ctx
        // (allocate auto-locals, bump the macro-execution counter).
        virtual macro_binding
            bind(const macro_def& def,
                 const std::vector<std::string>& args,
                 expand_context& ctx) const = 0;

        // Substitute parameters / counters into one macro body line.
        virtual std::string
            expand_line(const std::string& body_line,
                        const macro_binding& binding,
                        const expand_context& ctx) const = 0;

        // .irp/.irpc with an empty value list: gas expands once (symbol null),
        // sdas takes no action.
        virtual bool irp_empty_emits_once() const = 0;

        // Handle a dialect-specific in-macro attribute directive (sdas
        // .narg/.nchr/.nval).  Returns the replacement line, or nullopt if the
        // operator is not such a directive.  gas has none.
        virtual std::optional<std::string>
            attr_directive(const std::string& op,
                           const std::string& operand,
                           const expand_context& ctx) const = 0;
    };

    std::unique_ptr<macro_dialect> make_macro_dialect(asm_mode mode);

    // -------------------------------------------------------------------------
    // Engine
    // -------------------------------------------------------------------------

    class macro_processor {
    public:
        explicit macro_processor(std::unique_ptr<macro_dialect> dialect);

        // Object-like defines (-D NAME=VALUE) consumed by .rept counts.
        void add_define(const std::string& name, const std::string& value);

        // Quick check: does this text contain any macro/repeat directive?
        // When false, callers should skip preprocessing entirely.
        static bool has_macro_directives(const std::string& text);

        // Expand all macros and repeat blocks.  Throws macro_error.
        src_lines run(const src_lines& input);

        // Register every macro definition in 'input' without expanding or
        // emitting anything.  Used by the translator so that a later
        // expand_fragment() can resolve calls.
        void scan_definitions(const src_lines& input);

        // Expand a single fragment (a call line or a complete repeat block)
        // against the already-registered macros.  Throws macro_error.
        src_lines expand_fragment(const src_lines& fragment);

        // Is 'name' a registered macro?
        bool knows_macro(const std::string& name) const
        {
            return find_macro(name) != nullptr;
        }

        const macro_dialect& dialect() const { return *dialect_; }

    private:
        std::unique_ptr<macro_dialect>      dialect_;
        std::map<std::string, macro_def>    macros_;
        std::map<std::string, std::string>  defines_;
        expand_context                      ctx_;

        // Recursive expansion of a line range into 'out'.
        void expand(const src_lines& lines, src_lines& out, int depth);

        // Collect a block body starting at 'lines[start]' (an opener); returns
        // the index of the matching terminator and fills 'body' with the
        // interior lines.
        size_t collect_block(const src_lines& lines, size_t start,
                             macro_dialect::block kind, src_lines& body) const;

        void expand_call(const macro_def& def, const src_line& call,
                         const std::string& label, src_lines& out, int depth);
        void expand_repeat(macro_dialect::block kind, const src_line& header,
                           const src_lines& body, src_lines& out, int depth);

        const macro_def* find_macro(const std::string& op) const;
        long eval_count(const src_line& header, const std::string& expr) const;
    };

    // -------------------------------------------------------------------------
    // Shared lexical helpers (used by engine and dialects)
    // -------------------------------------------------------------------------
    namespace mtext {
        std::string ltrim(const std::string& s);
        std::string rtrim(const std::string& s);
        std::string trim(const std::string& s);
        std::string lower(const std::string& s);

        // Split a leading "label:" / "label::" off the line.  Returns the
        // label text (with its colon(s), empty if none) and the remainder.
        struct label_split { std::string label; std::string rest; };
        label_split split_label(const std::string& line);

        // First whitespace-delimited word of 's' (the operator field) and the
        // remaining operand text.
        struct op_split { std::string op; std::string operand; };
        op_split split_operator(const std::string& s);

        bool is_ident_char(char c);
    }

} // namespace xas

#endif // XAS_FRONTEND_MACRO_HPP
