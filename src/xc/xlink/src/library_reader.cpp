// library_reader.cpp
//
// Legacy library reader implementations.  The actual reading logic has
// been moved to libxbfd (lib/xbfd/ar_reader.cpp).  These thin stubs
// keep the xlink::library_reader interface intact for any code that
// might still reference it directly, delegating to bfd::bfd::open_r().
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
#include <xlink/library_reader.hpp>
#include <xlink/errors.hpp>
#include <xbfd/bfd.hpp>

namespace xlink {

    bool text_index_library_reader::can_read(
        const std::filesystem::path&,
        const std::string& data) const
    {
        return !(data.size() >= 8 && data.compare(0, 8, "!<arch>\n") == 0);
    }

    std::vector<lib_member> text_index_library_reader::read(
        const std::filesystem::path& path,
        const std::string&) const
    {
        auto arc = bfd::bfd::open_r(path);
        if (!arc->check_format(bfd::format::archive))
            throw parse_error("not a library: " + path.string());

        std::vector<lib_member> result;
        for (const auto& m : arc->members()) {
            lib_member lm;
            lm.path     = m.path;
            if (m.data.has_value())
                lm.contents = *m.data;
            result.push_back(std::move(lm));
        }
        return result;
    }

    bool ar_library_reader::can_read(
        const std::filesystem::path&,
        const std::string& data) const
    {
        return data.size() >= 8 && data.compare(0, 8, "!<arch>\n") == 0;
    }

    std::vector<lib_member> ar_library_reader::read(
        const std::filesystem::path& path,
        const std::string&) const
    {
        return text_index_library_reader{}.read(path, {});
    }

} // namespace xlink
