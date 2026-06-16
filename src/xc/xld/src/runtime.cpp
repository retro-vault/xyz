//
// SDCC runtime directory helpers
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
#include <algorithm>
#include <cctype>
#include <optional>
#include <system_error>
#include <vector>

#include <xld/runtime.h>
#include <xld/errors.h>

#ifndef XLD_DEFAULT_PLATFORM
#define XLD_DEFAULT_PLATFORM "cpm3"
#endif

namespace xld {

    static std::filesystem::path normalize_path(const std::filesystem::path& path)
    {
        std::error_code ec;
        auto normalized = std::filesystem::weakly_canonical(path, ec);
        if (!ec)
            return normalized;
        return path.lexically_normal();
    }

    static std::optional<std::filesystem::path> resolve_process_executable()
    {
        std::error_code ec;
        auto proc_self = std::filesystem::read_symlink("/proc/self/exe", ec);
        if (!ec && !proc_self.empty())
            return normalize_path(proc_self);
        return std::nullopt;
    }

    static bool has_extension_ci(const std::filesystem::path& path,
                                 const std::string& ext)
    {
        std::string got = path.extension().string();
        std::transform(got.begin(), got.end(), got.begin(), ::tolower);
        return got == ext;
    }

    static bool path_is_directory(const std::filesystem::path& path)
    {
        std::error_code ec;
        return std::filesystem::exists(path, ec)
            && std::filesystem::is_directory(path, ec);
    }

    static bool is_target_runtime_dir(const std::filesystem::path& runtime_dir);
    static std::filesystem::path resolve_shared_library_dir(
        const std::filesystem::path& runtime_dir);

    static std::string strip_target_arch_prefix(std::string target_name)
    {
        if (target_name.rfind("z80-", 0) == 0)
            return target_name.substr(4);
        return target_name;
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
        const std::vector<std::filesystem::path> candidates = {
            runtime_dir / "libruntime.a",
            runtime_dir / "libruntime.lib",
            runtime_dir / "z80.lib",
            runtime_dir / "runtime.lib",
        };

        for (const auto& candidate : candidates) {
            std::error_code ec;
            if (std::filesystem::exists(candidate, ec)
                && std::filesystem::is_regular_file(candidate, ec)) {
                return normalize_path(candidate);
            }
        }

        throw xld_error("no runtime archive found in SDCC runtime directory: "
                          + runtime_dir.string());
    }

    static std::optional<std::filesystem::path> resolve_optional_stdlib_lib(
        const std::filesystem::path& runtime_dir)
    {
        std::vector<std::filesystem::path> candidates = {
            runtime_dir / "libc.a",
            runtime_dir / "libc.lib",
            runtime_dir / "z80" / "libc.a",
            runtime_dir / "z80" / "libc.lib",
        };

        if (runtime_dir.filename() == "runtime"
            && runtime_dir.parent_path().filename() == "xcc"
            && runtime_dir.parent_path().parent_path().filename() == "libexec") {
            auto prefix = runtime_dir.parent_path().parent_path().parent_path();
            candidates.push_back(prefix / "lib" / "libc.a");
            candidates.push_back(prefix / "lib" / "libc.lib");
            candidates.push_back(prefix / "lib" / "z80" / "libc.a");
            candidates.push_back(prefix / "lib" / "z80" / "libc.lib");
        }

        for (const auto& candidate : candidates) {
            std::error_code ec;
            if (std::filesystem::exists(candidate, ec)
                && std::filesystem::is_regular_file(candidate, ec)) {
                return normalize_path(candidate);
            }
        }
        return std::nullopt;
    }

    static std::optional<std::string> selected_target_name(
        const cli_options& opts)
    {
        if (opts.platform_name.has_value() && !opts.platform_name->empty())
            return *opts.platform_name;

        if (!opts.invocation_target.empty()) {
            return opts.invocation_target;
        }

        return std::nullopt;
    }

    static std::optional<std::filesystem::path> resolve_optional_platform_lib(
        const std::filesystem::path& runtime_dir,
        const cli_options& opts)
    {
        std::vector<std::filesystem::path> candidates;
        auto add_named_candidates = [&](const std::string& platform_name) {
            const auto short_name = strip_target_arch_prefix(platform_name);
            if (short_name.empty())
                return;
            candidates.push_back(runtime_dir / ("lib" + short_name + ".a"));
            candidates.push_back(runtime_dir / ("lib" + short_name + ".lib"));
        };

        if (auto selected = selected_target_name(opts)) {
            add_named_candidates(*selected);
        } else {
            add_named_candidates(XLD_DEFAULT_PLATFORM);
        }
        candidates.push_back(runtime_dir / "platform.lib");
        candidates.push_back(runtime_dir / "sys.lib");

        for (const auto& candidate : candidates) {
            std::error_code ec;
            if (std::filesystem::exists(candidate, ec)
                && std::filesystem::is_regular_file(candidate, ec)) {
                return normalize_path(candidate);
            }
        }
        return std::nullopt;
    }

    static bool runtime_dir_satisfies(const cli_options& opts,
                                      const std::filesystem::path& runtime_dir)
    {
        if (!path_is_directory(runtime_dir))
            return false;

        if (!opts.no_stdlib && !opts.no_startfiles) {
            try {
                (void)resolve_crt0(runtime_dir);
            } catch (const xld_error&) {
                return false;
            }
        }

        if (!opts.no_stdlib) {
            try {
                (void)resolve_runtime_lib(resolve_shared_library_dir(runtime_dir));
            } catch (const xld_error&) {
                return false;
            }
        }

        return true;
    }

    std::optional<std::filesystem::path> runtime::find_default_sdcc_runtime_dir(
        const cli_options& opts,
        const std::filesystem::path& executable_path)
    {
        const auto normalized_executable = normalize_path(executable_path);
        const auto prefix = normalized_executable.parent_path().parent_path();

        if (auto target_name = selected_target_name(opts)) {
            auto candidate = prefix / "targets" / *target_name / "lib";
            if (runtime_dir_satisfies(opts, candidate))
                return normalize_path(candidate);
        }

        const std::vector<std::filesystem::path> candidates = {
            prefix / "z80" / "lib",
            prefix / "lib",
            prefix / "libexec" / "xcc" / "runtime",
        };

        for (const auto& candidate : candidates) {
            if (runtime_dir_satisfies(opts, candidate))
                return normalize_path(candidate);
        }
        return std::nullopt;
    }

    static bool is_target_runtime_dir(const std::filesystem::path& runtime_dir)
    {
        return runtime_dir.filename() == "lib"
            && runtime_dir.parent_path().parent_path().filename() == "targets";
    }

    static std::filesystem::path resolve_shared_library_dir(
        const std::filesystem::path& runtime_dir)
    {
        if (is_target_runtime_dir(runtime_dir)) {
            return normalize_path(
                runtime_dir.parent_path().parent_path().parent_path() / "lib");
        }
        return runtime_dir;
    }

    void runtime::apply_sdcc_runtime(cli_options& opts) {
        if (opts.mode == link_mode::gnu)
            return;

        if (!opts.sdcc_runtime_dir.has_value()
            && !opts.disable_default_sdcc_runtime) {
            if (auto executable = resolve_process_executable()) {
                opts.sdcc_runtime_dir = find_default_sdcc_runtime_dir(
                    opts, *executable);
            }
        }
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
            if (auto platform = resolve_optional_platform_lib(runtime_dir, opts))
                opts.input_files.push_back(*platform);

            const auto shared_library_dir = resolve_shared_library_dir(runtime_dir);
            if (auto stdlib = resolve_optional_stdlib_lib(shared_library_dir))
                opts.input_files.push_back(*stdlib);

            auto lib = resolve_runtime_lib(shared_library_dir);
            opts.input_files.push_back(lib);
        }
    }

} // namespace xld
