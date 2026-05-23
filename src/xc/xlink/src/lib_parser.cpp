//
// SDCC .lib parser/dispatcher
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
#include <fstream>
#include <iterator>
#include <vector>

#include <xlink/lib_parser.hpp>
#include <xlink/errors.hpp>

namespace xlink {

    std::vector<lib_member> lib_parser::parse(const std::filesystem::path& path)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open())
            throw parse_error("cannot open library file: " + path.string());

        std::string data((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());

        ar_library_reader ar_reader;
        text_index_library_reader text_reader;
        std::vector<const library_reader*> readers = {
            &ar_reader,
            &text_reader,
        };

        for (const auto* reader : readers) {
            if (reader->can_read(path, data))
                return reader->read(path, data);
        }

        throw parse_error("unsupported library format: " + path.string());
    }

} // namespace xlink
