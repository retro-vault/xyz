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
#include <xbfd/lscript.h>

#ifndef XLD_DEFAULT_PLATFORM
#define XLD_DEFAULT_PLATFORM "none"
#endif

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
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
#ifdef _WIN32
        std::vector<char> path_buf(MAX_PATH);
        for (;;) {
            const DWORD len = ::GetModuleFileNameA(
                nullptr, path_buf.data(), static_cast<DWORD>(path_buf.size()));
            if (len == 0)
                return std::nullopt;
            if (len < path_buf.size())
                return normalize_path(
                    std::filesystem::path(std::string(path_buf.data(), len)));
            path_buf.resize(path_buf.size() * 2);
        }
#else
        auto proc_self = std::filesystem::read_symlink("/proc/self/exe", ec);
        if (!ec && !proc_self.empty())
            return normalize_path(proc_self);
        return std::nullopt;
#endif
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
    static std::optional<std::string> selected_target_name(
        const cli_options& opts);
    static std::string strip_target_arch_prefix(std::string target_name);

    static output_format to_xld_output_format(
        xbfd::lscript_output_format fmt,
        const std::filesystem::path& script_path)
    {
        switch (fmt) {
        case xbfd::lscript_output_format::xl:
            return output_format::xl;
        case xbfd::lscript_output_format::bin:
            return output_format::bin;
        case xbfd::lscript_output_format::ihx:
            return output_format::ihx;
        case xbfd::lscript_output_format::elf:
            return output_format::elf;
        default:
            return output_format::xl;
        }
    }

    static std::string strip_target_arch_prefix(std::string target_name)
    {
        if (target_name.rfind("z80-", 0) == 0)
            return target_name.substr(4);
        return target_name;
    }

    static std::filesystem::path resolve_crt0(
        const std::filesystem::path& runtime_dir,
        const cli_options& opts)
    {
        std::vector<std::filesystem::path> preferred;
        auto add_named_candidate = [&](const std::string& target_name) {
            const auto short_name = strip_target_arch_prefix(target_name);
            if (!short_name.empty())
                preferred.push_back(runtime_dir / ("crt0-" + short_name + ".rel"));
        };

        if (auto selected = selected_target_name(opts))
            add_named_candidate(*selected);
        else
            add_named_candidate(XLD_DEFAULT_PLATFORM);

        preferred.push_back(runtime_dir / "crt0.rel");

        for (const auto& candidate : preferred) {
            if (std::filesystem::exists(candidate))
                return candidate;
        }

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

    static std::optional<std::filesystem::path> resolve_default_linker_script(
        const std::filesystem::path& runtime_dir,
        const cli_options& opts)
    {
        std::vector<std::filesystem::path> candidates;
        const char* default_name =
            opts.mode == link_mode::gnu ? "linker.ld" : "linker.lk";
        const char* platform_name =
            opts.mode == link_mode::gnu ? "platform.ld" : "platform.lk";
        const char* ext = opts.mode == link_mode::gnu ? ".ld" : ".lk";

        auto add_named_candidates = [&](const std::string& target_name) {
            const auto short_name = strip_target_arch_prefix(target_name);
            if (short_name.empty())
                return;
            candidates.push_back(runtime_dir / ("linker-" + short_name + ext));
            candidates.push_back(runtime_dir / (short_name + ext));
        };

        if (auto selected = selected_target_name(opts))
            add_named_candidates(*selected);
        else
            add_named_candidates(XLD_DEFAULT_PLATFORM);

        candidates.push_back(runtime_dir / default_name);
        candidates.push_back(runtime_dir / platform_name);

        for (const auto& candidate : candidates) {
            std::error_code ec;
            if (std::filesystem::exists(candidate, ec)
                && std::filesystem::is_regular_file(candidate, ec)) {
                return normalize_path(candidate);
            }
        }
        return std::nullopt;
    }

    static void apply_default_linker_script(cli_options& opts,
                                            const std::filesystem::path& runtime_dir)
    {
        if (opts.script_file.has_value())
            return;

        auto script_path = resolve_default_linker_script(runtime_dir, opts);
        if (!script_path.has_value())
            return;

        try {
            auto script = xbfd::lscript::open(
                *script_path,
                opts.mode == link_mode::gnu
                    ? xbfd::lscript_mode::gnu
                    : xbfd::lscript_mode::sdcc);

            opts.script_file = *script_path;

            if (script->entry_symbol().has_value()
                && !opts.entry_symbol_explicit) {
                opts.entry_symbol = *script->entry_symbol();
            }
            if (script->output_format().has_value()
                && !opts.format_explicit) {
                opts.format = to_xld_output_format(*script->output_format(),
                                                   *script_path);
            }
            if (script->output_range().has_value()
                && !opts.output_range_explicit) {
                opts.output_range = xld::address_range{
                    script->output_range()->start,
                    script->output_range()->end
                };
            }
            for (const auto& [area_name, base] : script->area_bases()) {
                if (opts.explicit_area_bases.find(area_name)
                    == opts.explicit_area_bases.end()) {
                    opts.area_bases[area_name] = base;
                }
            }
            for (const auto& area_name : script->area_order()) {
                if (std::find(opts.area_order.begin(),
                              opts.area_order.end(),
                              area_name) == opts.area_order.end()) {
                    opts.area_order.push_back(area_name);
                }
            }
            for (const auto& area_name : script->load_copy_areas()) {
                if (std::find(opts.load_copy_areas.begin(),
                              opts.load_copy_areas.end(),
                              area_name) == opts.load_copy_areas.end()) {
                    opts.load_copy_areas.push_back(area_name);
                }
            }
            for (const auto& range : script->reserved_ranges()) {
                opts.reserved_ranges.push_back(xld::address_range{
                    range.start,
                    range.end
                });
            }
        } catch (const xbfd::lscript_error& e) {
            throw xld_error(e.what());
        }
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
                (void)resolve_crt0(runtime_dir, opts);
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

        apply_default_linker_script(opts, runtime_dir);

        if (!opts.no_stdlib && !opts.no_startfiles) {
            auto crt0 = resolve_crt0(runtime_dir, opts);
            opts.input_files.insert(opts.input_files.begin(), crt0);
        }

        if (!opts.no_stdlib) {
            if (auto platform = resolve_optional_platform_lib(runtime_dir, opts))
                opts.input_files.push_back(*platform);

            const auto shared_library_dir = resolve_shared_library_dir(runtime_dir);
            auto add_library_search_path = [&](const std::filesystem::path& path) {
                auto normalized = normalize_path(path);
                for (const auto& existing : opts.library_search_paths) {
                    if (existing == normalized)
                        return;
                }
                opts.library_search_paths.push_back(std::move(normalized));
            };
            add_library_search_path(shared_library_dir);
            add_library_search_path(runtime_dir);

            if (auto stdlib = resolve_optional_stdlib_lib(shared_library_dir))
                opts.input_files.push_back(*stdlib);

            auto lib = resolve_runtime_lib(shared_library_dir);
            opts.input_files.push_back(lib);
        }
    }

} // namespace xld
