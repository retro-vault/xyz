// macro_translate.h
//
// Macro-aware source translation for the --format path.
//
// When pretty-printing/converting assembly from one dialect to another, macro
// and repeat constructs are handled as follows:
//
//   * A construct whose structure is expressible in the target dialect is
//     re-emitted as a native target-dialect macro/repeat (framing + parameter
//     references translated; the body is converted through the normal source
//     emitter so its directives/instructions are converted too).
//
//   * A construct that relies on features the target dialect cannot express
//     (e.g. gas \@ / \+ / :vararg, or sdas ' concatenation / \num / ?auto-local
//     / .narg) is expanded in place, so the converted output still assembles.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#ifndef XAS_BACKEND_MACRO_TRANSLATE_HPP
#define XAS_BACKEND_MACRO_TRANSLATE_HPP

#include <map>
#include <set>
#include <string>

#include <xas/cli.h>
#include <xas/frontend/macro.h>

namespace xas {

    class macro_translator {
    public:
        macro_translator(asm_mode src_mode, output_format dst_format);

        // Convert 'lines' (in src_mode dialect) to dst_format source text,
        // translating compatible macros and expanding the rest.
        std::string translate(const src_lines& lines);

    private:
        asm_mode        src_mode_;
        output_format   dst_format_;
        bool            src_gas_;
        bool            dst_gas_;
        bool            same_dialect_;
        macro_processor proc_;                     // source dialect + all defs
        std::map<std::string, macro_def> translated_; // name -> def (native emit)

        const macro_dialect& dia() const { return proc_.dialect(); }

        // Render a run of ordinary source lines through lex/parse/emit.
        std::string render_code(const src_lines& lines) const;

        // Block collection mirroring the engine, using the source dialect.
        size_t collect(const src_lines& lines, size_t start,
                       macro_dialect::block kind, src_lines& body) const;

        // Translatability predicates.
        bool def_translatable(const macro_def& def) const;
        bool body_translatable(const src_lines& body,
                               const std::vector<std::string>& params) const;

        // Native emission of a translatable construct.
        std::string emit_macro(const macro_def& def) const;
        std::string emit_repeat(macro_dialect::block kind,
                                const std::string& operand,
                                const src_lines& body) const;
        std::string emit_call(const std::string& label, const macro_def& def,
                              const std::string& operand) const;

        // Convert macro/loop body lines to target text (param refs -> sentinels,
        // render through emitter, sentinels -> target refs).  params lists the
        // dummy-argument names visible in the body.
        std::string convert_body(const src_lines& body,
                                 const std::vector<std::string>& params) const;
    };

} // namespace xas

#endif // XAS_BACKEND_MACRO_TRANSLATE_HPP
