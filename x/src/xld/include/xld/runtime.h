//
// SDCC runtime directory helpers
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
#ifndef XLINK_RUNTIME_HPP
#define XLINK_RUNTIME_HPP

#include <xld/cli.h>

namespace xld {

    class runtime {
    public:
        static std::optional<std::filesystem::path> find_default_sdcc_runtime_dir(
            const cli_options& opts,
            const std::filesystem::path& executable_path);
        static void apply_sdcc_runtime(cli_options& opts);
    };

} // namespace xld

#endif // XLINK_RUNTIME_HPP
