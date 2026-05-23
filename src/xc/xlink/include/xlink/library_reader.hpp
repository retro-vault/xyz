//
// library readers
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
#ifndef XLINK_LIBRARY_READER_HPP
#define XLINK_LIBRARY_READER_HPP

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace xlink {

    struct lib_member {
        std::filesystem::path path;
        std::optional<std::string> contents;
    };

    class library_reader {
    public:
        virtual ~library_reader() = default;

        virtual bool can_read(const std::filesystem::path& path,
                              const std::string& data) const = 0;

        virtual std::vector<lib_member> read(
            const std::filesystem::path& path,
            const std::string& data) const = 0;
    };

    class text_index_library_reader final : public library_reader {
    public:
        bool can_read(const std::filesystem::path& path,
                      const std::string& data) const override;

        std::vector<lib_member> read(
            const std::filesystem::path& path,
            const std::string& data) const override;
    };

    class ar_library_reader final : public library_reader {
    public:
        bool can_read(const std::filesystem::path& path,
                      const std::string& data) const override;

        std::vector<lib_member> read(
            const std::filesystem::path& path,
            const std::string& data) const override;
    };

} // namespace xlink

#endif // XLINK_LIBRARY_READER_HPP
