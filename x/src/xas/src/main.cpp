// main.cpp
//
// xas — Z80 assembler for the xyz toolchain.
// Supports SDCC sdasz80 syntax (--mode=sdcc) and GNU gas syntax (--mode=gnu).
// Outputs SDCC .rel or ELF32 z80-elf object files.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

#include <xas/cli.h>
#include <xas/errors.h>
#include <xas/backend/source_emitter.h>
#include <xas/backend/macro_translate.h>
#include <xas/frontend/lexer.h>
#include <xas/frontend/macro.h>
#include <xas/frontend/parser.h>
#include <xas/backend/emitter.h>

namespace xas {
    // Defined in codegen.cpp.
    void assemble(const stmt_list& stmts, emitter& emit,
                  const std::string& module_name,
                  const std::string& src_file,
                  asm_mode mode,
                  const std::vector<std::string>& defines);

    // Defined in rel_emitter.cpp / elf_emitter.cpp.
    std::unique_ptr<emitter> make_rel_emitter(const std::string& path);
    std::unique_ptr<emitter> make_elf_emitter(const std::string& path);
}

namespace {

    std::string read_text_file(const std::filesystem::path& path)
    {
        std::ifstream in(path);
        if (!in.is_open())
            throw std::runtime_error("cannot open '" + path.string() + "'");
        std::stringstream buffer;
        buffer << in.rdbuf();
        return buffer.str();
    }

    std::string format_predefines(const std::vector<std::string>& defines,
                                  xas::output_format format)
    {
        std::string out;
        for (const std::string& def : defines) {
            if (def.empty())
                continue;
            size_t eq = def.find('=');
            std::string name = eq == std::string::npos ? def : def.substr(0, eq);
            std::string value = eq == std::string::npos ? "1" : def.substr(eq + 1);
            if (name.empty())
                continue;
            if (format == xas::output_format::gnu)
                out += ".set " + name + ", " + value + "\n";
            else
                out += ".define " + name + " = " + value + "\n";
        }
        return out;
    }

    bool starts_with(const std::string& s, size_t pos, const char* text)
    {
        for (size_t i = 0; text[i] != '\0'; ++i) {
            if (pos + i >= s.size() || s[pos + i] != text[i])
                return false;
        }
        return true;
    }

    std::optional<std::string> parse_include_target(const std::string& line)
    {
        size_t i = 0;
        while (i < line.size() && (line[i] == ' ' || line[i] == '\t' || line[i] == '\r'))
            ++i;
        if (i >= line.size() || line[i] != '.')
            return std::nullopt;
        ++i;
        if (!starts_with(line, i, "include"))
            return std::nullopt;
        i += sizeof("include") - 1;
        if (i < line.size()) {
            char c = line[i];
            if (!(c == ' ' || c == '\t' || c == '\r' || c == '"'))
                return std::nullopt;
        }

        while (i < line.size() && (line[i] == ' ' || line[i] == '\t' || line[i] == '\r'))
            ++i;
        if (i >= line.size() || line[i] != '"')
            return std::nullopt;
        ++i;

        std::string target;
        while (i < line.size() && line[i] != '"') {
            if (line[i] == '\\' && i + 1 < line.size()) {
                ++i;
                switch (line[i]) {
                    case 'n':  target += '\n'; break;
                    case 't':  target += '\t'; break;
                    case 'r':  target += '\r'; break;
                    case '\\': target += '\\'; break;
                    case '"':  target += '"'; break;
                    case '0':  target += '\0'; break;
                    default:   target += line[i]; break;
                }
                ++i;
                continue;
            }
            target += line[i++];
        }
        if (i >= line.size() || line[i] != '"')
            return std::nullopt;
        ++i;

        while (i < line.size() && (line[i] == ' ' || line[i] == '\t' || line[i] == '\r'))
            ++i;
        if (i == line.size())
            return target;
        if (line[i] == ';')
            return target;
        if (line[i] == '/' && i + 1 < line.size() && line[i + 1] == '/')
            return target;
        return std::nullopt;
    }

    std::filesystem::path resolve_include_path(
        const std::string& target,
        const std::filesystem::path& including_file,
        const std::vector<std::string>& include_dirs)
    {
        namespace fs = std::filesystem;
        fs::path candidate(target);
        if (candidate.is_absolute()) {
            if (fs::exists(candidate))
                return candidate.lexically_normal();
        } else {
            std::vector<fs::path> roots;
            if (!including_file.empty()) {
                // A bare input name (for example, "main.s") has an empty
                // parent_path(), but its quoted includes are still relative
                // to the current source directory.  Keep that directory in
                // the search list so paths such as "../z80.inc" work when
                // xas is invoked from beside the input file.
                roots.push_back(including_file.has_parent_path()
                                    ? including_file.parent_path()
                                    : fs::path("."));
            }
            for (const std::string& dir : include_dirs)
                roots.emplace_back(dir);
            for (const fs::path& root : roots) {
                fs::path joined = (root / candidate).lexically_normal();
                if (fs::exists(joined))
                    return joined;
            }
        }
        throw std::runtime_error("cannot resolve include \"" + target + "\"");
    }

    xas::src_lines load_source_tree(
        const std::filesystem::path& path,
        const std::vector<std::string>& include_dirs,
        std::unordered_set<std::string>& include_stack)
    {
        namespace fs = std::filesystem;

        fs::path normalized = path.lexically_normal();
        std::string key = normalized.string();
        if (!include_stack.insert(key).second)
            throw std::runtime_error("circular include detected for '" + key + "'");

        std::string source;
        try {
            source = read_text_file(normalized);
        } catch (const std::exception& e) {
            include_stack.erase(key);
            throw;
        }

        xas::src_lines lines;
        int lineno = 1;
        for (size_t i = 0; i < source.size(); ) {
            size_t nl = source.find('\n', i);
            std::string text = (nl == std::string::npos)
                                   ? source.substr(i) : source.substr(i, nl - i);

            if (auto include_target = parse_include_target(text)) {
                fs::path include_path =
                    resolve_include_path(*include_target, normalized, include_dirs);
                auto nested = load_source_tree(include_path, include_dirs, include_stack);
                lines.insert(lines.end(), nested.begin(), nested.end());
            } else {
                lines.push_back({ text, normalized.string(), lineno });
            }

            ++lineno;
            if (nl == std::string::npos)
                break;
            i = nl + 1;
        }

        include_stack.erase(key);
        return lines;
    }

    std::string join_source_lines(const xas::src_lines& lines)
    {
        std::string out;
        for (const xas::src_line& sl : lines) {
            out += sl.text;
            out += '\n';
        }
        return out;
    }

} // namespace

int main(int argc, char** argv)
{
    try {
        auto opts = xas::parse_args(argc, argv);

        // Split source into physical lines (origin tracking for diagnostics).
        auto split_lines = [&](const std::string& s) {
            xas::src_lines lines;
            int lineno = 1;
            for (size_t i = 0; i < s.size(); ) {
                size_t nl = s.find('\n', i);
                std::string text = (nl == std::string::npos)
                                       ? s.substr(i) : s.substr(i, nl - i);
                lines.push_back({ text, opts.input, lineno++ });
                if (nl == std::string::npos) break;
                i = nl + 1;
            }
            return lines;
        };

        std::string source = read_text_file(opts.input);
        const bool has_macros =
            xas::macro_processor::has_macro_directives(source);

        // --- Source-to-source conversion (--format) --------------------------
        if (opts.format.has_value()) {
            std::string document_text;
            if (has_macros) {
                // Translate compatible macros to the target dialect; expand the
                // rest so the converted output still assembles.
                xas::macro_translator tr(opts.mode, *opts.format);
                document_text = tr.translate(split_lines(source));
                std::ofstream out(opts.output);
                if (!out.is_open()) {
                    std::cerr << "xas: error: cannot create '" << opts.output << "'\n";
                    return 1;
                }
                out << format_predefines(opts.defines, *opts.format);
                out << document_text;
                return 0;
            }
            std::istringstream lex_in(source);
            xas::lexer lex;
            auto tokens = lex.tokenise(opts.input, lex_in);
            xas::parser par;
            auto stmts = par.parse(tokens, opts.mode);
            auto emit = xas::make_source_emitter(*opts.format);
            auto document = emit->emit(stmts);
            std::ofstream out(opts.output);
            if (!out.is_open()) {
                std::cerr << "xas: error: cannot create '" << opts.output << "'\n";
                return 1;
            }
            out << format_predefines(opts.defines, *opts.format);
            xas::write_formatted_document(out, document);
            return 0;
        }

        // --- Assembly path: fully expand macros first ------------------------
        std::unordered_set<std::string> include_stack;
        xas::src_lines source_lines =
            load_source_tree(std::filesystem::path(opts.input),
                             opts.include_dirs, include_stack);
        source = join_source_lines(source_lines);
        const bool assembly_has_macros =
            xas::macro_processor::has_macro_directives(source);

        if (assembly_has_macros) {
            xas::macro_processor mp(xas::make_macro_dialect(opts.mode));
            for (const std::string& d : opts.defines) {
                size_t eq = d.find('=');
                if (eq == std::string::npos) mp.add_define(d, "1");
                else mp.add_define(d.substr(0, eq), d.substr(eq + 1));
            }
            source_lines = mp.run(source_lines);
            source = join_source_lines(source_lines);
        }

        // Lex.
        std::istringstream lex_in(source);
        xas::lexer lex;
        auto tokens = lex.tokenise(opts.input, lex_in);

        // Parse.
        xas::parser par;
        auto stmts = par.parse(tokens, opts.mode);

        {
            // Choose emitter.
            std::unique_ptr<xas::emitter> emit;
            if (opts.mode == xas::asm_mode::gnu)
                emit = xas::make_elf_emitter(opts.output);
            else
                emit = xas::make_rel_emitter(opts.output);

            // Code generation (two-pass).
            std::string module = std::filesystem::path(opts.input).stem().string();
            xas::assemble(stmts, *emit, module, opts.input, opts.mode,
                          opts.defines);
        }

        return 0;

    } catch (const xas::asm_error& e) {
        std::cerr << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "xas: error: " << e.what() << "\n";
        return 1;
    }
}
