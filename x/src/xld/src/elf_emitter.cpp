// elf_emitter.cpp
//
// primary linked ELF output writer
//
// MIT License (see: LICENSE)
#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <xbfd/xbfd.h>

#include <xld/debug_info.h>
#include <xld/elf_emitter.h>
#include <xld/errors.h>

namespace xld {

    namespace {

        struct output_section {
            std::string name;
            uint32_t start = 0;
            uint32_t end = 0;
            bool has_data = false;
            bool bss = false;
        };

        static bool starts_with(const std::string& value,
                                const std::string& prefix)
        {
            return value.rfind(prefix, 0) == 0;
        }

        static bool is_bss_area_name(const std::string& name) {
            return name == "_BSS"
                || name == "_HEAP"
                || name == ".bss"
                || starts_with(name, ".bss.")
                || starts_with(name, ".tbss.");
        }

        static bool is_data_area_name(const std::string& name) {
            return name == "_DATA"
                || name == "_BSS"
                || name == "_HEAP"
                || name == "_INITIALIZED"
                || name == "_INITIALIZER"
                || name == "_GSINIT"
                || name == "_GSFINAL"
                || name == "_DABS"
                || name == "_CABS"
                || name == ".data"
                || name == ".bss"
                || name == ".rodata"
                || name == ".tdata"
                || name == ".tbss"
                || starts_with(name, "_DATA_BANK_")
                || starts_with(name, ".data.")
                || starts_with(name, ".bss.")
                || starts_with(name, ".rodata.")
                || starts_with(name, ".tdata.")
                || starts_with(name, ".tbss.");
        }

        static bool is_readonly_area_name(const std::string& name) {
            return name == "_CONST"
                || name == ".rodata"
                || starts_with(name, "_CONST_BANK_")
                || starts_with(name, ".rodata.");
        }

        static bool is_internal_symbol_name(const std::string& name,
                                            const std::string& module_name)
        {
            if (name.empty())
                return true;
            if (name.size() > 3 && name[0] == '.'
                && name[1] == '_' && name[2] == '_') {
                return true;
            }
            const std::string local_start = "F" + module_name + "$";
            const std::string local_end = "XF" + module_name + "$";
            return name.rfind(local_start, 0) == 0
                || name.rfind(local_end, 0) == 0;
        }

        static xbfd::section_flags section_flags_for(const std::string& name) {
            auto flags = xbfd::section_flags::alloc;
            if (is_bss_area_name(name))
                return flags | xbfd::section_flags::data
                             | xbfd::section_flags::never_load;
            flags = flags | xbfd::section_flags::load;
            if (is_data_area_name(name) && !is_readonly_area_name(name))
                flags = flags | xbfd::section_flags::data;
            if (!is_data_area_name(name))
                flags = flags | xbfd::section_flags::code;
            if (is_readonly_area_name(name))
                flags = flags | xbfd::section_flags::readonly;
            return flags;
        }

        static symbol_kind infer_symbol_kind(const module* mod,
                                             const symbol& sym)
        {
            if (sym.kind() != symbol_kind::notype)
                return sym.kind();
            if (sym.area_index() < 0
                || sym.area_index() >= static_cast<int>(mod->areas().size())) {
                return symbol_kind::notype;
            }
            return is_data_area_name(mod->areas()[sym.area_index()].name())
                ? symbol_kind::object
                : symbol_kind::function;
        }

        static xbfd::symbol_flags symbol_flags_for(const symbol& sym,
                                                   symbol_kind kind)
        {
            auto flags = sym.is_global() ? xbfd::symbol_flags::global
                                         : xbfd::symbol_flags::local;
            if (sym.is_weak())
                flags = flags | xbfd::symbol_flags::weak;
            if (sym.is_absolute())
                flags = flags | xbfd::symbol_flags::absolute;
            if (kind == symbol_kind::function)
                flags = flags | xbfd::symbol_flags::function;
            else if (kind == symbol_kind::object)
                flags = flags | xbfd::symbol_flags::object;
            return flags;
        }

        static std::vector<output_section> collect_output_sections(
            const link_context& ctx)
        {
            std::map<std::string, output_section> by_name;
            for (const auto& mod : ctx.modules) {
                for (const auto& area : mod->areas()) {
                    if (!area.placed_addr().has_value() || area.size() == 0)
                        continue;
                    auto& out = by_name[area.name()];
                    out.name = area.name();
                    const uint32_t start = area.placed_addr().value();
                    const uint32_t end = start + area.size();
                    if (!out.has_data) {
                        out.start = start;
                        out.end = end;
                    } else {
                        out.start = std::min(out.start, start);
                        out.end = std::max(out.end, end);
                    }
                    out.has_data = true;
                    out.bss = out.bss || is_bss_area_name(area.name());
                }
            }

            std::vector<output_section> result;
            for (auto& [_, sec] : by_name)
                result.push_back(sec);
            std::sort(result.begin(), result.end(),
                      [](const auto& a, const auto& b) {
                          if (a.start != b.start)
                              return a.start < b.start;
                          return a.name < b.name;
                      });
            return result;
        }

        static const output_section* find_output_section(
            const std::vector<output_section>& sections,
            const std::string& name)
        {
            for (const auto& sec : sections)
                if (sec.name == name)
                    return &sec;
            return nullptr;
        }

        static void add_sections(xbfd::object& obj,
                                 const link_context& ctx,
                                 const std::vector<output_section>& sections)
        {
            for (const auto& out : sections) {
                xbfd::section sec;
                sec.name = out.name;
                sec.flags = section_flags_for(out.name);
                sec.vma = out.start;
                sec.size = out.end - out.start;
                if (!out.bss) {
                    sec.data.assign(sec.size, 0x00);
                    for (uint32_t addr = out.start; addr < out.end; ++addr) {
                        if (addr < ctx.code_buffer.size())
                            sec.data[addr - out.start] = ctx.code_buffer[addr];
                    }
                }
                obj.sections.push_back(std::move(sec));
            }
        }

        static void add_symbols(xbfd::object& obj,
                                const link_context& ctx,
                                const std::vector<output_section>& sections)
        {
            std::set<std::string> emitted_globals;

            for (const auto& mod : ctx.modules) {
                for (const auto& sym : mod->symbols()) {
                    if (!sym.is_def())
                        continue;
                    if (is_internal_symbol_name(sym.name(), mod->name()))
                        continue;
                    if (sym.is_global()) {
                        auto selected = ctx.global_symbols.find(sym.name());
                        if (selected == ctx.global_symbols.end()
                            || selected->second.first != mod.get()
                            || selected->second.second != sym.index()) {
                            continue;
                        }
                        emitted_globals.insert(sym.name());
                    }

                    const auto kind = infer_symbol_kind(mod.get(), sym);
                    auto flags = symbol_flags_for(sym, kind);
                    uint32_t value =
                        debug_info_builder::symbol_absolute_addr(mod.get(), sym);
                    std::string section_name;
                    if (!sym.is_absolute()
                        && sym.area_index() >= 0
                        && sym.area_index() < static_cast<int>(mod->areas().size())) {
                        const auto& area = mod->areas()[sym.area_index()];
                        if (find_output_section(sections, area.name()) != nullptr)
                            section_name = area.name();
                    } else {
                        flags = flags | xbfd::symbol_flags::absolute;
                    }

                    obj.symbols.push_back({
                        sym.name(),
                        flags,
                        value,
                        section_name,
                        sym.size()
                    });
                }
            }

            for (const auto& [name, address] : ctx.linker_symbols) {
                if (!emitted_globals.insert(name).second)
                    continue;
                obj.symbols.push_back({
                    name,
                    xbfd::symbol_flags::global | xbfd::symbol_flags::absolute,
                    address,
                    "",
                    0
                });
            }
        }

        static xbfd::debug_lang to_xbfd_language(debug_language lang) {
            switch (lang) {
            case debug_language::c:
                return xbfd::debug_lang::c;
            case debug_language::assembly:
                return xbfd::debug_lang::assembly;
            default:
                return xbfd::debug_lang::unknown;
            }
        }

        static xbfd::debug_info build_xbfd_debug_info(
            const linked_debug_info& linked)
        {
            xbfd::debug_info result;
            for (std::size_t i = 0; i < linked.modules.size(); ++i) {
                const auto& info = linked.modules[i];
                const uint32_t file_id = static_cast<uint32_t>(i + 1);
                result.files.push_back({
                    file_id,
                    debug_info_builder::normalize_path_string(
                        info.source_path.empty() ? info.mod->path()
                                                 : info.source_path),
                    to_xbfd_language(info.language)
                });
                for (const auto& [address, line] : info.line_by_address)
                    result.lines.push_back({address, line, file_id});
                for (const auto& [_, fn] : info.functions) {
                    result.functions.push_back({
                        fn.display_name,
                        fn.start_address,
                        std::max<uint32_t>(fn.end_address,
                                           fn.start_address + 1u),
                        file_id,
                        fn.line.value_or(0),
                        fn.calling_convention
                    });
                }
            }
            return result;
        }

    } // namespace

    void elf_emitter::emit(const std::filesystem::path& path,
                           const link_context& ctx,
                           bool include_debug)
    {
        xbfd::object obj;
        obj.module_name = path.stem().string();
        obj.format = xbfd::obj_format::executable;
        obj.flavour = xbfd::obj_flavour::elf;
        obj.entry = ctx.entry_point;

        const auto sections = collect_output_sections(ctx);
        add_sections(obj, ctx, sections);
        add_symbols(obj, ctx, sections);

        if (include_debug)
            obj.debug = build_xbfd_debug_info(
                debug_info_builder::build(path, ctx));

        try {
            xbfd::elf_writer writer;
            writer.write(path.string(), obj);
        } catch (const xbfd::bfd_error& e) {
            throw xld_error(e.what());
        }
    }

} // namespace xld
