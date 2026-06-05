// lib_parser.cpp
//
// Reads .lib archives (both SDCC text-index and GNU ar formats) by
// delegating to libxbfd, then converts the result to xld::lib_member.
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
#include <fstream>
#include <iterator>

#include <xld/lib_parser.h>
#include <xld/errors.h>
#include <xbfd/xbfd.h>

namespace xld {

    std::vector<lib_member> lib_parser::parse(const std::filesystem::path& path)
    {
        try {
            auto arc = bfd::bfd::open_r(path);
            if (!arc->check_format(bfd::format::archive))
                throw parse_error("not a library file: " + path.string());

            std::vector<lib_member> result;
            for (const auto& m : arc->members()) {
                lib_member lm;
                lm.path     = m.path;
                if (m.data.has_value())
                    lm.contents = *m.data;
                result.push_back(std::move(lm));
            }
            return result;
        } catch (const bfd::bfd_error& e) {
            throw parse_error(std::string(e.what()));
        }
    }

} // namespace xld
