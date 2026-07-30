// rel_parser.cpp
//
// Reads SDCC .rel object files by delegating to libxbfd, then converts
// the generic bfd::bfd representation into the xld-specific module
// hierarchy (module / area / symbol / text_record).
//
// The conversion maps:
//   bfd::section   → xld::area  + xld::text_record
//   bfd::symbol    → xld::symbol
//   bfd::reloc     → xld::reloc_entry (indices rebuilt from names)
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
#include <algorithm>
#include <fstream>
#include <set>
#include <sstream>

#include <xld/rel_parser.h>
#include <xld/errors.h>
#include <xbfd/xbfd.h>

namespace xld {

    namespace {

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
                || name.rfind("_DATA_BANK_", 0) == 0
                || name == ".data"
                || name == ".bss"
                || name == ".rodata"
                || name == ".tdata"
                || name == ".tbss"
                || name.rfind(".data.", 0) == 0
                || name.rfind(".bss.", 0) == 0
                || name.rfind(".rodata.", 0) == 0
                || name.rfind(".tdata.", 0) == 0
                || name.rfind(".tbss.", 0) == 0;
        }

        static symbol_kind bfd_symbol_kind(const bfd::symbol& sym) {
            if (bfd::has_flag(sym.flags, bfd::symbol_flags::function))
                return symbol_kind::function;
            if (bfd::has_flag(sym.flags, bfd::symbol_flags::object))
                return symbol_kind::object;
            return symbol_kind::notype;
        }

        static bool is_code_like_symbol_name(
            const std::string& name,
            const std::set<std::string>& defined_names)
        {
            if (name.empty())
                return false;

            if (name.rfind("__xcc_", 0) == 0 || name.rfind("__cmp_", 0) == 0)
                return true;

            if (name[0] == '_') {
                const auto end_name = "__" + name.substr(1) + "_end";
                if (defined_names.find(end_name) != defined_names.end())
                    return true;
            }

            if (name.rfind("__", 0) == 0
                && name.size() > 6
                && name.substr(name.size() - 4) == "_end") {
                return true;
            }

            return false;
        }

        // Map bfd::reloc_type to xld reloc_mode flags.
        static reloc_mode bfd_to_xlink_reloc_mode(bfd::reloc_type t,
                                                   bool sym_relative)
        {
            reloc_mode m = reloc_mode::none;
            switch (t) {
                case bfd::reloc_type::z80_16:
                    m = reloc_mode::word;
                    break;
                case bfd::reloc_type::z80_8:
                    m = reloc_mode::none; // byte reloc — default
                    break;
                case bfd::reloc_type::z80_pc8:
                    m = reloc_mode::pc_rel;
                    break;
                case bfd::reloc_type::z80_16_msb:
                    m = reloc_mode::msb;
                    break;
                default:
                    m = reloc_mode::word;
                    break;
            }
            if (sym_relative)
                m = m | reloc_mode::sym;
            return m;
        }

        // Convert a bfd::bfd (object) into an xld::module.
        static std::shared_ptr<module> bfd_to_module(
            const bfd::bfd& obj,
            const std::string& path_str)
        {
            auto mod = std::make_shared<module>(obj.module_name(),
                                                std::filesystem::path(path_str));

            // Build area list from bfd sections.
            for (const auto& sec : obj.sections()) {
                if (sec.size > 0xFFFFu) {
                    throw parse_error(
                        path_str + ": section '" + sec.name
                        + "' exceeds the 16-bit Z80 address space");
                }
                uint8_t raw_flags = 0;
                if (bfd::has_flag(sec.flags, bfd::section_flags::overlay))
                    raw_flags |= 0x01;
                if (bfd::has_flag(sec.flags, bfd::section_flags::abs))
                    raw_flags |= 0x08;

                auto af = static_cast<area_flags>(raw_flags);
                std::optional<uint16_t> org;
                if (bfd::has_flag(sec.flags, bfd::section_flags::abs))
                    org = static_cast<uint16_t>(sec.vma);

                int idx = static_cast<int>(mod->areas().size());
                mod->areas().emplace_back(sec.name,
                                          static_cast<uint16_t>(sec.size),
                                          af, idx, org);
            }

            // Build symbol list.
            std::set<std::string> defined_names;
            for (const auto& sym : obj.symbols()) {
                if (sym.is_defined())
                    defined_names.insert(sym.name);
            }

            std::string first_code_area;
            bool has_nonzero_data_area = false;
            for (const auto& area : mod->areas()) {
                if (!is_data_area_name(area.name()) && first_code_area.empty())
                    first_code_area = area.name();
                if (is_data_area_name(area.name()) && area.size() != 0)
                    has_nonzero_data_area = true;
            }

            for (const auto& sym : obj.symbols()) {
                bool is_def = sym.is_defined();
                symbol_type stype = is_def ? symbol_type::def : symbol_type::ref;

                // Find area index for defined symbols.
                int area_idx = -1;
                if (is_def && !sym.section_name.empty()) {
                    std::string section_name = sym.section_name;
                    if (!first_code_area.empty()
                        && is_data_area_name(section_name)
                        && is_code_like_symbol_name(sym.name, defined_names)) {
                        section_name = first_code_area;
                    } else if (!first_code_area.empty()
                               && !has_nonzero_data_area
                               && (section_name.empty()
                                   || is_data_area_name(section_name))) {
                        section_name = first_code_area;
                    }

                    auto& areas = mod->areas();
                    for (size_t i = 0; i < areas.size(); ++i) {
                        if (areas[i].name() == section_name) {
                            area_idx = static_cast<int>(i);
                            break;
                        }
                    }
                }

                int idx = static_cast<int>(mod->symbols().size());
                xld::symbol xs(sym.name, stype,
                                  static_cast<uint16_t>(sym.value),
                                  idx, area_idx, sym.is_absolute(),
                                  bfd_symbol_kind(sym),
                                  static_cast<uint16_t>(
                                      std::min<uint64_t>(sym.size, 0xFFFFu)),
                                  sym.is_global(),
                                  bfd::has_flag(sym.flags,
                                                bfd::symbol_flags::weak));
                xs.set_absolute(sym.is_absolute());
                xs.set_owner(mod.get());
                mod->symbols().push_back(xs);
            }

            // Build text records and relocations.
            for (size_t si = 0; si < obj.sections().size(); ++si) {
                const auto& sec = obj.sections()[si];
                const auto& data = sec.data;
                if (data.empty()) continue;

                text_record tr;
                tr.area_index = static_cast<int>(si);
                tr.offset     = 0;
                tr.data       = data;

                for (const auto& r : sec.relocs) {
                    reloc_entry re;
                    re.mode        = bfd_to_xlink_reloc_mode(r.type, r.sym_relative);
                    re.offset_in_t = static_cast<uint16_t>(r.offset);

                    // Rebuild index.
                    re.ref_index = 0;
                    if (r.sym_relative) {
                        auto& syms = mod->symbols();
                        for (size_t i = 0; i < syms.size(); ++i) {
                            if (syms[i].name() == r.name) {
                                re.ref_index = static_cast<int>(i);
                                break;
                            }
                        }
                    } else {
                        auto& areas = mod->areas();
                        for (size_t i = 0; i < areas.size(); ++i) {
                            if (areas[i].name() == r.name) {
                                re.ref_index = static_cast<int>(i);
                                break;
                            }
                        }
                    }
                    tr.relocs.push_back(re);
                }

                mod->texts().push_back(std::move(tr));
            }

            return mod;
        }

    } // anonymous namespace

    // -------------------------------------------------------------------------
    // Public API
    // -------------------------------------------------------------------------

    std::shared_ptr<module> rel_parser::parse(
        const std::filesystem::path& path)
    {
        try {
            auto obj = bfd::bfd::open_r(path);
            if (!obj->check_format(bfd::format::object))
                throw parse_error("not an object file: " + path.string());
            return bfd_to_module(*obj, path.string());
        } catch (const bfd::format_error& e) {
            throw parse_error(std::string(e.what()));
        } catch (const bfd::io_error& e) {
            throw parse_error(std::string(e.what()));
        }
    }

    std::shared_ptr<module> rel_parser::parse(const std::string& source_name,
                                              std::istream& input)
    {
        try {
            auto obj = bfd::bfd::open_r_stream(source_name, input);
            if (!obj->check_format(bfd::format::object))
                throw parse_error("not an object file: " + source_name);
            return bfd_to_module(*obj, source_name);
        } catch (const bfd::format_error& e) {
            throw parse_error(std::string(e.what()));
        } catch (const bfd::io_error& e) {
            throw parse_error(std::string(e.what()));
        }
    }

    std::vector<std::string> rel_parser::scan_defs(
        const std::filesystem::path& path)
    {
        try {
            auto obj = bfd::bfd::open_r(path);
            if (!obj->check_format(bfd::format::object)) return {};
            std::vector<std::string> defs;
            for (const auto& sym : obj->symbols()) {
                if (sym.is_defined() && sym.is_global())
                    defs.push_back(sym.name);
            }
            return defs;
        } catch (...) {
            return {};
        }
    }

    std::vector<std::string> rel_parser::scan_defs(
        const std::string& source_name,
        std::istream& input)
    {
        try {
            auto obj = bfd::bfd::open_r_stream(source_name, input);
            if (!obj->check_format(bfd::format::object)) return {};
            std::vector<std::string> defs;
            for (const auto& sym : obj->symbols()) {
                if (sym.is_defined() && sym.is_global())
                    defs.push_back(sym.name);
            }
            return defs;
        } catch (...) {
            return {};
        }
    }

} // namespace xld
