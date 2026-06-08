//
// GNU ELF + DWARF2 debug emitter
//
// MIT License (see: LICENSE)
//
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include <xbfd/xbfd.h>

#include <xld/debug_info.h>
#include <xld/elf_debug_emitter.h>
#include <xld/errors.h>

namespace xld {

    namespace {

        struct image_view {
            uint16_t start = 0;
            std::vector<uint8_t> bytes;
        };

        struct file_record {
            uint32_t id = 0;
            const linked_module_debug_info* info = nullptr;
            std::filesystem::path path;
        };

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
                || name == ".bss";
        }

        static bool is_bss_area_name(const std::string& name) {
            return name == "_BSS"
                || name == "_HEAP"
                || name == ".bss";
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
            if (name.rfind("A$", 0) == 0 || name.rfind("C$", 0) == 0
                || name.rfind("G$", 0) == 0 || name.rfind("XG$", 0) == 0) {
                return true;
            }

            const std::string local_start = "F" + module_name + "$";
            const std::string local_end = "XF" + module_name + "$";
            if (name.rfind(local_start, 0) == 0 || name.rfind(local_end, 0) == 0)
                return true;

            return false;
        }

        static image_view build_image(const link_context& ctx) {
            image_view result;

            uint16_t start = 0;
            uint16_t end = 0;
            if (ctx.output_range.has_value()) {
                start = ctx.output_range->start;
                end = ctx.output_range->end;
                if (start > end)
                    throw xld_error("invalid binary output range");
            } else if (!ctx.code_buffer.empty()) {
                end = static_cast<uint16_t>(ctx.code_buffer.size() - 1);
            }

            result.start = start;
            const uint32_t out_size = static_cast<uint32_t>(end - start + 1);
            result.bytes.assign(out_size, 0x00);
            for (uint32_t addr = start; addr <= end; ++addr) {
                if (addr < ctx.code_buffer.size())
                    result.bytes[addr - start] = ctx.code_buffer[addr];
            }
            return result;
        }

        static void add_image_section(xbfd::object& obj, const image_view& image) {
            xbfd::section sec;
            sec.name = ".text";
            sec.flags = xbfd::section_flags::alloc
                      | xbfd::section_flags::load
                      | xbfd::section_flags::code;
            sec.vma = image.start;
            sec.size = image.bytes.size();
            sec.data = image.bytes;
            obj.sections.push_back(std::move(sec));
        }

        static xbfd::symbol_flags xbfd_symbol_flags(bool is_global,
                                                    bool is_function)
        {
            auto flags = xbfd::symbol_flags::absolute;
            flags = flags | (is_global ? xbfd::symbol_flags::global
                                       : xbfd::symbol_flags::local);
            if (is_function)
                flags = flags | xbfd::symbol_flags::function;
            return flags;
        }

        static void add_final_symbols(xbfd::object& obj, const link_context& ctx) {
            std::set<std::string> seen;

            for (const auto& [name, ref] : ctx.global_symbols) {
                const module* mod = ref.first;
                const auto& sym = mod->symbol_by_index(ref.second);
                if (is_internal_symbol_name(name, mod->name()))
                    continue;
                if (!seen.insert(name).second)
                    continue;

                const auto& areas = mod->areas();
                bool is_function = sym.area_index() >= 0
                    && sym.area_index() < static_cast<int>(areas.size())
                    && !is_data_area_name(areas[sym.area_index()].name());
                obj.symbols.push_back({
                    name,
                    xbfd_symbol_flags(true, is_function),
                    debug_info_builder::symbol_absolute_addr(mod, sym),
                    ""
                });
            }

            for (const auto& [name, address] : ctx.linker_symbols) {
                if (!seen.insert(name).second)
                    continue;
                obj.symbols.push_back({
                    name,
                    xbfd::symbol_flags::global | xbfd::symbol_flags::absolute,
                    address,
                    ""
                });
            }
        }

        static std::vector<debug_function_info> synthesize_module_functions(
            const linked_module_debug_info& info)
        {
            if (!info.functions.empty()) {
                std::vector<debug_function_info> result;
                for (const auto& [_, fn] : info.functions)
                    result.push_back(fn);
                std::sort(result.begin(), result.end(),
                          [](const auto& a, const auto& b) {
                              if (a.start_address != b.start_address)
                                  return a.start_address < b.start_address;
                              return a.display_name < b.display_name;
                          });
                return result;
            }

            std::vector<debug_function_info> result;
            if (info.mod == nullptr)
                return result;

            const auto& mod = *info.mod;
            for (const auto& sym : mod.symbols()) {
                if (!sym.is_def())
                    continue;
                if (is_internal_symbol_name(sym.name(), mod.name()))
                    continue;
                if (sym.area_index() < 0
                    || sym.area_index() >= static_cast<int>(mod.areas().size())) {
                    continue;
                }
                const auto& area = mod.area_by_index(sym.area_index());
                if (is_data_area_name(area.name()))
                    continue;

                debug_function_info fn;
                fn.display_name = sym.name();
                fn.fallback_name = sym.name();
                fn.start_address = debug_info_builder::symbol_absolute_addr(
                    &mod, sym);
                fn.end_address = static_cast<uint32_t>(fn.start_address + 1);
                result.push_back(std::move(fn));
            }

            std::sort(result.begin(), result.end(),
                      [](const auto& a, const auto& b) {
                          if (a.start_address != b.start_address)
                              return a.start_address < b.start_address;
                          return a.display_name < b.display_name;
                      });
            for (std::size_t i = 0; i < result.size(); ++i) {
                if (i + 1 < result.size() && result[i + 1].start_address > result[i].start_address)
                    result[i].end_address = result[i + 1].start_address;
            }
            return result;
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
            std::set<std::string> seen_debug_symbols;

            std::vector<file_record> files;
            files.reserve(linked.modules.size());
            for (std::size_t i = 0; i < linked.modules.size(); ++i) {
                const auto& info = linked.modules[i];
                xbfd::debug_source_file file;
                file.id = static_cast<uint32_t>(i + 1);
                file.path = debug_info_builder::normalize_path_string(
                    info.source_path.empty() ? info.mod->path() : info.source_path);
                file.language = to_xbfd_language(info.language);
                result.files.push_back(file);
                files.push_back({file.id, &info, file.path});
            }

            for (const auto& file : files) {
                const auto& info = *file.info;
                for (const auto& [address, line] : info.line_by_address) {
                    xbfd::debug_line row;
                    row.address = address;
                    row.line = line;
                    row.file_id = file.id;
                    result.lines.push_back(row);
                }

                auto functions = synthesize_module_functions(info);
                for (const auto& fn : functions) {
                    xbfd::debug_function out_fn;
                    out_fn.name = fn.display_name;
                    out_fn.start = fn.start_address;
                    out_fn.end = std::max<uint32_t>(fn.end_address,
                                                   fn.start_address + 1u);
                    out_fn.file_id = file.id;
                    out_fn.line = fn.line.value_or(0);
                    out_fn.convention = fn.calling_convention;
                    result.functions.push_back(out_fn);

                    if (seen_debug_symbols.insert(out_fn.name).second) {
                        result.symbols.push_back({out_fn.name, out_fn.start});
                    }
                }
            }

            std::sort(result.lines.begin(), result.lines.end(),
                      [](const auto& a, const auto& b) {
                          if (a.file_id != b.file_id)
                              return a.file_id < b.file_id;
                          if (a.address != b.address)
                              return a.address < b.address;
                          return a.line < b.line;
                      });

            std::sort(result.functions.begin(), result.functions.end(),
                      [](const auto& a, const auto& b) {
                          if (a.file_id != b.file_id)
                              return a.file_id < b.file_id;
                          if (a.start != b.start)
                              return a.start < b.start;
                          return a.name < b.name;
                      });

            return result;
        }

    } // namespace

    void elf_debug_emitter::emit(const std::filesystem::path& path,
                                 const std::filesystem::path& image_path,
                                 const link_context& ctx) const
    {
        const auto linked = debug_info_builder::build(image_path, ctx);
        auto image = build_image(ctx);

        xbfd::object obj;
        obj.module_name = path.stem().string();
        obj.format = xbfd::obj_format::object;
        obj.flavour = xbfd::obj_flavour::elf;
        add_image_section(obj, image);
        add_final_symbols(obj, ctx);
        obj.debug = build_xbfd_debug_info(linked);

        try {
            xbfd::elf_writer writer;
            writer.write(path.string(), obj);
        } catch (const xbfd::bfd_error& e) {
            throw xld_error(e.what());
        }
    }

} // namespace xld
