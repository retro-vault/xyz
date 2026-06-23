// source_emitter.h
//
// Structured source emitters for xas.  These consume the parsed assembly AST
// and produce a typed fragment document that can later be rendered as plain
// text, HTML, XML, or other styled output.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#ifndef XAS_BACKEND_SOURCE_EMITTER_HPP
#define XAS_BACKEND_SOURCE_EMITTER_HPP

#include <memory>
#include <ostream>

#include <xas/cli.h>
#include <xas/frontend/ast.h>
#include <xz80/formatter.h>

namespace xas {

    class source_emitter {
    public:
        virtual ~source_emitter() = default;
        virtual xz80::formatted_document emit(const stmt_list& stmts) const = 0;
    };

    std::unique_ptr<source_emitter> make_source_emitter(output_format format);
    void write_formatted_document(std::ostream& out,
                                  const xz80::formatted_document& document);

} // namespace xas

#endif // XAS_BACKEND_SOURCE_EMITTER_HPP
