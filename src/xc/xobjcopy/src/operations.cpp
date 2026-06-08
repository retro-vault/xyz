//
// operations.cpp
//
// xobjcopy conversion and strip-debug operations.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <algorithm>
#include <filesystem>
#include <set>
#include <string>

#include <xbfd/xbfd.h>

#include <xobjcopy/errors.h>
#include <xobjcopy/operations.h>

namespace xobjcopy {

    namespace {

        static bool is_debug_section_name(const std::string& name) {
            return name.rfind(".debug", 0) == 0
                || name.rfind(".zdebug", 0) == 0
                || name == ".line"
                || name == ".stab"
                || name == ".stabstr";
        }

        static xbfd::obj_flavour to_xbfd_flavour(target_kind kind) {
            switch (kind) {
            case target_kind::rel:
                return xbfd::obj_flavour::rel;
            case target_kind::elf:
                return xbfd::obj_flavour::elf;
            case target_kind::ar_text:
                return xbfd::obj_flavour::ar_text;
            case target_kind::ar_binary:
                return xbfd::obj_flavour::ar_binary;
            }
            return xbfd::obj_flavour::unknown;
        }

        static target_kind from_flavour(xbfd::obj_flavour flavour) {
            switch (flavour) {
            case xbfd::obj_flavour::rel:
                return target_kind::rel;
            case xbfd::obj_flavour::elf:
                return target_kind::elf;
            case xbfd::obj_flavour::ar_text:
                return target_kind::ar_text;
            case xbfd::obj_flavour::ar_binary:
                return target_kind::ar_binary;
            default:
                throw error("unsupported input flavour");
            }
        }

        static void strip_debug_from_object(xbfd::object& obj) {
            std::set<std::string> removed_sections;
            obj.sections.erase(
                std::remove_if(
                    obj.sections.begin(),
                    obj.sections.end(),
                    [&](const xbfd::section& sec) {
                        const bool remove =
                            xbfd::has_flag(sec.flags, xbfd::section_flags::debugging)
                            || is_debug_section_name(sec.name);
                        if (remove)
                            removed_sections.insert(sec.name);
                        return remove;
                    }),
                obj.sections.end());

            obj.symbols.erase(
                std::remove_if(
                    obj.symbols.begin(),
                    obj.symbols.end(),
                    [&](const xbfd::symbol& sym) {
                        if (removed_sections.find(sym.section_name)
                            != removed_sections.end()) {
                            return true;
                        }
                        if (!sym.name.empty() && sym.name.rfind(".Ldebug", 0) == 0)
                            return true;
                        return false;
                    }),
                obj.symbols.end());

            obj.debug = {};
        }

        static xbfd::obj_flavour require_output_flavour(
            const cli_options& opts, xbfd::obj_format input_format)
        {
            if (!opts.output_target.has_value())
                throw usage_error(
                    "cannot infer output target; use -O or an output extension");

            const auto flavour = to_xbfd_flavour(opts.output_target.value());
            if (input_format == xbfd::obj_format::object
                && (flavour == xbfd::obj_flavour::ar_text
                    || flavour == xbfd::obj_flavour::ar_binary)) {
                throw usage_error("object input requires object output target");
            }
            if (input_format == xbfd::obj_format::archive
                && (flavour == xbfd::obj_flavour::rel
                    || flavour == xbfd::obj_flavour::elf)) {
                throw usage_error("archive input requires archive output target");
            }
            return flavour;
        }

        static void write_object(const std::filesystem::path& path,
                                 xbfd::object obj,
                                 xbfd::obj_flavour flavour)
        {
            obj.format = xbfd::obj_format::object;
            obj.flavour = flavour;
            auto out = bfd::bfd::open_w(path, flavour);
            out->object() = std::move(obj);
            out->close();
        }

        static void write_archive(const std::filesystem::path& path,
                                  xbfd::object obj,
                                  xbfd::obj_flavour flavour)
        {
            obj.format = xbfd::obj_format::archive;
            obj.flavour = flavour;
            auto out = bfd::bfd::create_archive(path, flavour);
            out->object() = std::move(obj);
            out->close();
        }

    } // namespace

    void run(const cli_options& opts) {
        try {
            auto in = bfd::bfd::open_r(opts.input_file);
            if (!in->check_format(xbfd::obj_format::object)
                && !in->check_format(xbfd::obj_format::archive)) {
                throw error("unsupported input format");
            }

            const auto input_format = in->get_format();
            const auto input_flavour = in->get_flavour();
            if (opts.input_target.has_value()
                && opts.input_target.value() != from_flavour(input_flavour)) {
                throw usage_error("input target does not match detected input format");
            }

            auto obj = in->object();
            if (opts.strip_debug) {
                if (input_format != xbfd::obj_format::object)
                    throw usage_error("--strip-debug currently supports object inputs only");
                strip_debug_from_object(obj);
            }

            const auto output_flavour = require_output_flavour(opts, input_format);
            if (input_format == xbfd::obj_format::object)
                write_object(opts.output_file, std::move(obj), output_flavour);
            else
                write_archive(opts.output_file, std::move(obj), output_flavour);
        } catch (const xbfd::bfd_error& e) {
            throw error(e.what());
        }
    }

} // namespace xobjcopy
