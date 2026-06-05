//
// SDCC runtime directory helpers
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
#include <algorithm>
#include <vector>

#include <xld/runtime.h>
#include <xld/errors.h>

namespace xld {

    static bool has_extension_ci(const std::filesystem::path& path,
                                 const std::string& ext)
    {
        std::string got = path.extension().string();
        std::transform(got.begin(), got.end(), got.begin(), ::tolower);
        return got == ext;
    }

    static std::filesystem::path resolve_crt0(
        const std::filesystem::path& runtime_dir)
    {
        auto preferred = runtime_dir / "crt0.rel";
        if (std::filesystem::exists(preferred))
            return preferred;

        std::vector<std::filesystem::path> candidates;
        for (const auto& entry : std::filesystem::directory_iterator(runtime_dir)) {
            if (!entry.is_regular_file())
                continue;

            const auto path = entry.path();
            if (!has_extension_ci(path, ".rel"))
                continue;

            std::string stem = path.stem().string();
            std::transform(stem.begin(), stem.end(), stem.begin(), ::tolower);
            if (stem.rfind("crt0", 0) == 0)
                candidates.push_back(path);
        }

        if (candidates.size() == 1)
            return candidates[0];

        if (candidates.empty()) {
            throw xld_error("no crt0.rel found in SDCC runtime directory: "
                              + runtime_dir.string());
        }

        throw xld_error("multiple crt0 candidates in SDCC runtime directory: "
                          + runtime_dir.string()
                          + " (use crt0.rel to disambiguate)");
    }

    static std::filesystem::path resolve_runtime_lib(
        const std::filesystem::path& runtime_dir)
    {
        auto preferred = runtime_dir / "z80.lib";
        if (std::filesystem::exists(preferred))
            return preferred;

        std::vector<std::filesystem::path> candidates;
        for (const auto& entry : std::filesystem::directory_iterator(runtime_dir)) {
            if (!entry.is_regular_file())
                continue;
            if (has_extension_ci(entry.path(), ".lib"))
                candidates.push_back(entry.path());
        }

        if (candidates.size() == 1)
            return candidates[0];

        if (candidates.empty()) {
            throw xld_error("no .lib found in SDCC runtime directory: "
                              + runtime_dir.string());
        }

        throw xld_error("multiple .lib files in SDCC runtime directory: "
                          + runtime_dir.string()
                          + " (use z80.lib to disambiguate)");
    }

    void runtime::apply_sdcc_runtime(cli_options& opts) {
        if (!opts.sdcc_runtime_dir.has_value())
            return;

        const auto& runtime_dir = opts.sdcc_runtime_dir.value();
        if (!std::filesystem::exists(runtime_dir)) {
            throw xld_error("SDCC runtime directory does not exist: "
                              + runtime_dir.string());
        }
        if (!std::filesystem::is_directory(runtime_dir)) {
            throw xld_error("SDCC runtime path is not a directory: "
                              + runtime_dir.string());
        }

        if (!opts.no_stdlib && !opts.no_startfiles) {
            auto crt0 = resolve_crt0(runtime_dir);
            opts.input_files.insert(opts.input_files.begin(), crt0);
        }

        if (!opts.no_stdlib) {
            auto lib = resolve_runtime_lib(runtime_dir);
            opts.input_files.push_back(lib);
        }
    }

} // namespace xld
