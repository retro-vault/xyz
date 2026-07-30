#include <array>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <rsp/rsp.h>
#include <xconfig/simple_config.h>
#include <xemu/xemu.h>

namespace {

#ifndef XTOOLS_VERSION
#define XTOOLS_VERSION "0.1.0"
#endif

enum class image_format {
    bin,
    ihx,
    elf
};

struct partial_store_config {
    std::optional<uint16_t> bank_count;
    std::optional<uint32_t> bank_size;
    std::optional<bool> writable;
};

struct partial_selector_config {
    std::optional<uint16_t> initial_value;
};

struct partial_window_config {
    std::optional<uint16_t> start;
    std::optional<uint16_t> end;
    std::optional<std::string> store;
    std::optional<uint16_t> fixed_bank;
    std::optional<std::string> selector;
    std::optional<uint32_t> bank_offset;
};

struct partial_port_rule_config {
    std::optional<uint16_t> port;
    std::optional<uint16_t> port_mask;
    std::optional<std::string> selector;
    std::optional<uint16_t> mask;
    std::optional<uint8_t> shift;
};

struct memory_map_builder {
    std::map<std::string, partial_store_config> stores;
    std::map<std::string, partial_selector_config> selectors;
    std::map<std::string, partial_window_config> windows;
    std::map<std::string, partial_port_rule_config> port_rules;
    bool touched = false;
};

struct options {
    std::string listen_host = "127.0.0.1";
    uint16_t listen_port = 9000;
    std::optional<std::filesystem::path> config_path;
    std::optional<std::filesystem::path> loaded_config_path;
    std::optional<std::filesystem::path> image_path;
    std::optional<uint16_t> image_entry;
    std::optional<xemu::memory_map_config> memory_map;
    image_format image = image_format::bin;
    uint16_t origin = 0x0000;
    std::optional<uint16_t> pc;
    uint16_t sp = 0xFFFF;
    bool emu_stdio = false;
    bool emu_exit_status = false;
    std::optional<uint16_t> stdin_port;
    std::optional<uint16_t> stdin_status_port;
    std::optional<uint16_t> stdin_data_port;
    std::optional<uint16_t> stdout_port;
    std::optional<std::filesystem::path> fs_root;
    std::optional<uint16_t> bank_port;
    std::optional<uint16_t> bank_count;
    std::optional<std::vector<uint8_t>> shared_pages;
    std::optional<std::vector<uint8_t>> banked_pages;
    std::size_t max_steps = 1'000'000;
    bool quiet = false;
    bool run_mode = false;
    bool show_help = false;
    bool show_version = false;
};

void print_help() {
    std::cout
        << "Usage: xemu [options]\n\n"
        << "X Tools Emulator (xemu) — Z80 emulator and RSP target\n\n"
        << "modes:\n"
        << "  default              start an RSP server for xgdb-compatible clients\n"
        << "  --run                execute immediately until HALT or the step limit\n\n"
        << "config:\n"
        << "  --config FILE        load defaults from FILE\n"
        << "                       auto-searches ./xemu.conf then ~/.config/x/xemu.conf\n"
        << "                       advanced banking uses store./window./selector./port_rule. keys\n"
        << "                       port_rule.*.port_mask can match partially decoded ports\n\n"
        << "options:\n"
        << "  --listen HOST:PORT   listen address (default 127.0.0.1:9000)\n"
        << "  --run                run immediately instead of waiting for a debugger\n"
        << "  --max-steps N        step budget for --run (default 1000000)\n"
        << "  --load-bin FILE      load raw binary into memory\n"
        << "  --load-ihx FILE      load Intel HEX records into memory\n"
        << "  --load-elf FILE      load ELF sections into memory\n"
        << "  --origin ADDR        binary load address (default 0x0000)\n"
        << "  --pc ADDR            initial program counter (default: ELF entry or origin)\n"
        << "  --sp ADDR            initial stack pointer (default 0xFFFF)\n"
        << "  --emu-stdio          map platform=emu stdio ports to host stdin/stdout\n"
        << "  --no-emu-stdio       disable platform=emu stdio even if enabled in config\n"
        << "  --emu-exit-status    return the platform=emu mailbox status to the host\n"
        << "  --fs-root DIR        map platform=emu file syscalls to host DIR\n"
        << "  --stdin-port ADDR    map single Z80 input port ADDR to host stdin\n"
        << "  --stdin-status-port ADDR\n"
        << "                       map Z80 status port ADDR to host stdin readiness\n"
        << "  --stdin-data-port ADDR\n"
        << "                       map Z80 data port ADDR to host stdin bytes\n"
        << "  --stdout-port ADDR   map Z80 port ADDR to host stdout\n"
        << "  --shared-pages LIST  compatibility banking shortcut, e.g. 0,1,3\n"
        << "  --banked-pages LIST  compatibility banking shortcut, e.g. 2\n"
        << "  --bank-count N       compatibility shortcut: number of banks (1..256)\n"
        << "  --bank-port ADDR     compatibility shortcut: OUT port selects active bank\n"
        << "  -q, --quiet          quiet startup\n"
        << "  --no-quiet           disable quiet mode even if enabled in config\n"
        << "  --version            print version\n"
        << "  -h, --help           show this help\n";
}

uint32_t parse_u32(const std::string& value) {
    char* end = nullptr;
    const unsigned long parsed = std::strtoul(value.c_str(), &end, 0);
    if (end == value.c_str() || *end != '\0')
        throw std::runtime_error("invalid number: " + value);
    return static_cast<uint32_t>(parsed);
}

bool parse_bool(const std::string& value, const std::string& field_name) {
    const auto key = xconfig::normalize_key(value);
    if (key == "1" || key == "true" || key == "yes" || key == "on")
        return true;
    if (key == "0" || key == "false" || key == "no" || key == "off")
        return false;
    throw std::runtime_error("invalid boolean for " + field_name + ": " + value);
}

std::pair<std::string, uint16_t> split_host_port(const std::string& value) {
    const auto colon = value.rfind(':');
    if (colon == std::string::npos)
        throw std::runtime_error("expected host:port");
    return {
        value.substr(0, colon),
        static_cast<uint16_t>(parse_u32(value.substr(colon + 1)))
    };
}

std::vector<std::string> split_dotted(std::string_view value) {
    std::vector<std::string> parts;
    std::size_t start = 0;
    while (start <= value.size()) {
        const auto dot = value.find('.', start);
        parts.emplace_back(value.substr(
            start,
            dot == std::string_view::npos ? std::string_view::npos : dot - start));
        if (dot == std::string_view::npos) {
            break;
        }
        start = dot + 1;
    }
    return parts;
}

std::vector<uint8_t> parse_page_list(const std::string& value) {
    std::vector<uint8_t> pages;
    std::size_t start = 0;

    while (start <= value.size()) {
        const auto comma = value.find(',', start);
        const auto token = xconfig::trim(value.substr(
            start,
            comma == std::string::npos ? std::string::npos : comma - start));
        if (!token.empty()) {
            const auto page = parse_u32(token);
            if (page >= xemu::banked_memory_config::page_count) {
                throw std::runtime_error("page index out of range: " + token);
            }
            pages.push_back(static_cast<uint8_t>(page));
        }

        if (comma == std::string::npos) {
            break;
        }
        start = comma + 1;
    }

    return pages;
}

std::pair<uint16_t, uint16_t> parse_address_range(const std::string& value) {
    const auto dots = value.find("..");
    if (dots != std::string::npos) {
        const auto lhs = xconfig::trim(value.substr(0, dots));
        const auto rhs = xconfig::trim(value.substr(dots + 2));
        return {
            static_cast<uint16_t>(parse_u32(lhs)),
            static_cast<uint16_t>(parse_u32(rhs))
        };
    }

    const auto dash = value.find('-');
    if (dash == std::string::npos) {
        throw std::runtime_error("expected address range START-END, got: " + value);
    }

    const auto lhs = xconfig::trim(value.substr(0, dash));
    const auto rhs = xconfig::trim(value.substr(dash + 1));
    return {
        static_cast<uint16_t>(parse_u32(lhs)),
        static_cast<uint16_t>(parse_u32(rhs))
    };
}

std::filesystem::path resolve_config_path(
    const std::filesystem::path& base_dir,
    const std::string& value)
{
    std::filesystem::path path(value);
    if (path.is_absolute()) {
        return path;
    }
    return (base_dir / path).lexically_normal();
}

bool has_help_flag(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            return true;
        }
    }
    return false;
}

std::optional<std::filesystem::path> discover_config_path(int argc, char* argv[]) {
    std::optional<std::filesystem::path> explicit_config;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--config") {
            if (++i >= argc) {
                throw std::runtime_error("--config requires a path");
            }
            explicit_config = argv[i];
        }
    }

    if (explicit_config.has_value()) {
        return explicit_config;
    }
    return xconfig::find_default_tool_config("xemu");
}

bool apply_memory_map_entry(
    memory_map_builder& builder,
    const std::string& raw_key,
    const std::string& value)
{
    const auto key = xconfig::normalize_key(raw_key);
    const auto parts = split_dotted(key);
    if (parts.empty()) {
        return false;
    }

    if (parts[0] == "store" && parts.size() == 3) {
        builder.touched = true;
        auto& store = builder.stores[parts[1]];
        if (parts[2] == "banks") {
            store.bank_count = static_cast<uint16_t>(parse_u32(value));
            return true;
        }
        if (parts[2] == "banksize" || parts[2] == "size") {
            store.bank_size = parse_u32(value);
            return true;
        }
        if (parts[2] == "writable" || parts[2] == "readonly") {
            const bool bool_value = parse_bool(value, raw_key);
            store.writable = parts[2] == "readonly" ? !bool_value : bool_value;
            return true;
        }
        throw std::runtime_error("unknown store config key: " + raw_key);
    }

    if (parts[0] == "selector" && (parts.size() == 2 || parts.size() == 3)) {
        builder.touched = true;
        auto& selector = builder.selectors[parts[1]];
        if (parts.size() == 2 || parts[2] == "initial" || parts[2] == "value") {
            selector.initial_value = static_cast<uint16_t>(parse_u32(value));
            return true;
        }
        throw std::runtime_error("unknown selector config key: " + raw_key);
    }

    if (parts[0] == "window" && parts.size() == 3) {
        builder.touched = true;
        auto& window = builder.windows[parts[1]];
        if (parts[2] == "range") {
            const auto [start, end] = parse_address_range(value);
            window.start = start;
            window.end = end;
            return true;
        }
        if (parts[2] == "store") {
            window.store = xconfig::trim(value);
            return true;
        }
        if (parts[2] == "fixedbank" || parts[2] == "bank") {
            window.fixed_bank = static_cast<uint16_t>(parse_u32(value));
            return true;
        }
        if (parts[2] == "selector") {
            window.selector = xconfig::trim(value);
            return true;
        }
        if (parts[2] == "bankoffset" || parts[2] == "offset") {
            window.bank_offset = parse_u32(value);
            return true;
        }
        throw std::runtime_error("unknown window config key: " + raw_key);
    }

    if ((parts[0] == "portrule" || parts[0] == "port_rule") && parts.size() == 3) {
        builder.touched = true;
        auto& rule = builder.port_rules[parts[1]];
        if (parts[2] == "port") {
            rule.port = static_cast<uint16_t>(parse_u32(value));
            return true;
        }
        if (parts[2] == "portmask" || parts[2] == "port_mask") {
            rule.port_mask = static_cast<uint16_t>(parse_u32(value));
            return true;
        }
        if (parts[2] == "selector") {
            rule.selector = xconfig::trim(value);
            return true;
        }
        if (parts[2] == "mask") {
            rule.mask = static_cast<uint16_t>(parse_u32(value));
            return true;
        }
        if (parts[2] == "shift") {
            rule.shift = static_cast<uint8_t>(parse_u32(value));
            return true;
        }
        throw std::runtime_error("unknown port_rule config key: " + raw_key);
    }

    return false;
}

std::optional<xemu::memory_map_config> resolve_memory_map(const memory_map_builder& builder) {
    if (!builder.touched) {
        return std::nullopt;
    }
    if (builder.stores.empty()) {
        throw std::runtime_error("memory map config must define at least one store");
    }
    if (builder.windows.empty()) {
        throw std::runtime_error("memory map config must define at least one window");
    }

    xemu::memory_map_config config;
    auto selectors = builder.selectors;

    for (const auto& [name, window] : builder.windows) {
        if (window.selector.has_value() && selectors.count(*window.selector) == 0) {
            selectors[*window.selector] = partial_selector_config{};
        }
    }
    for (const auto& [name, rule] : builder.port_rules) {
        if (rule.selector.has_value() && selectors.count(*rule.selector) == 0) {
            selectors[*rule.selector] = partial_selector_config{};
        }
    }

    for (const auto& [name, store] : builder.stores) {
        xemu::memory_store_config cfg;
        cfg.name = name;
        cfg.bank_count = store.bank_count.value_or(1);
        cfg.bank_size = store.bank_size.value_or(0x10000u);
        cfg.writable = store.writable.value_or(true);
        config.stores.push_back(std::move(cfg));
    }

    for (const auto& [name, selector] : selectors) {
        xemu::memory_selector_config cfg;
        cfg.name = name;
        cfg.initial_value = selector.initial_value.value_or(0);
        config.selectors.push_back(std::move(cfg));
    }

    for (const auto& [name, window] : builder.windows) {
        if (!window.start.has_value() || !window.end.has_value()) {
            throw std::runtime_error("window." + name + ".range is required");
        }
        if (!window.store.has_value()) {
            throw std::runtime_error("window." + name + ".store is required");
        }

        xemu::memory_window_config cfg;
        cfg.start = window.start.value();
        cfg.end = window.end.value();
        cfg.store = window.store.value();
        cfg.fixed_bank = window.fixed_bank;
        cfg.selector = window.selector;
        cfg.bank_offset = window.bank_offset.value_or(0);
        config.windows.push_back(std::move(cfg));
    }

    for (const auto& [name, rule] : builder.port_rules) {
        if (!rule.port.has_value()) {
            throw std::runtime_error("port_rule." + name + ".port is required");
        }
        if (!rule.selector.has_value()) {
            throw std::runtime_error("port_rule." + name + ".selector is required");
        }

        xemu::memory_port_rule_config cfg;
        cfg.port = rule.port.value();
        cfg.port_mask = rule.port_mask.value_or(0xFFFF);
        cfg.selector = rule.selector.value();
        cfg.mask = rule.mask.value_or(0x00FF);
        cfg.shift = rule.shift.value_or(0);
        config.port_rules.push_back(std::move(cfg));
    }

    return config;
}

void apply_config_entry(
    options& opts,
    memory_map_builder& map_builder,
    const std::string& raw_key,
    const std::string& value,
    const std::filesystem::path& base_dir)
{
    if (apply_memory_map_entry(map_builder, raw_key, value)) {
        return;
    }

    const auto key = xconfig::normalize_key(raw_key);
    if (key == "listen") {
        const auto [host, port] = split_host_port(value);
        opts.listen_host = host;
        opts.listen_port = port;
    } else if (key == "run") {
        opts.run_mode = parse_bool(value, raw_key);
    } else if (key == "maxsteps") {
        opts.max_steps = static_cast<std::size_t>(parse_u32(value));
    } else if (key == "loadbin") {
        opts.image_path = resolve_config_path(base_dir, value);
        opts.image = image_format::bin;
    } else if (key == "loadihx") {
        opts.image_path = resolve_config_path(base_dir, value);
        opts.image = image_format::ihx;
    } else if (key == "loadelf") {
        opts.image_path = resolve_config_path(base_dir, value);
        opts.image = image_format::elf;
    } else if (key == "origin") {
        opts.origin = static_cast<uint16_t>(parse_u32(value));
    } else if (key == "pc") {
        opts.pc = static_cast<uint16_t>(parse_u32(value));
    } else if (key == "sp") {
        opts.sp = static_cast<uint16_t>(parse_u32(value));
    } else if (key == "emustdio") {
        opts.emu_stdio = parse_bool(value, raw_key);
    } else if (key == "stdinport") {
        opts.stdin_port = static_cast<uint16_t>(parse_u32(value));
    } else if (key == "stdinstatusport") {
        opts.stdin_status_port = static_cast<uint16_t>(parse_u32(value));
    } else if (key == "stdindataport") {
        opts.stdin_data_port = static_cast<uint16_t>(parse_u32(value));
    } else if (key == "stdoutport") {
        opts.stdout_port = static_cast<uint16_t>(parse_u32(value));
    } else if (key == "fsroot") {
        opts.fs_root = resolve_config_path(base_dir, value);
    } else if (key == "quiet") {
        opts.quiet = parse_bool(value, raw_key);
    } else if (key == "bankport") {
        opts.bank_port = static_cast<uint16_t>(parse_u32(value));
    } else if (key == "bankcount") {
        opts.bank_count = static_cast<uint16_t>(parse_u32(value));
    } else if (key == "sharedpages") {
        opts.shared_pages = parse_page_list(value);
    } else if (key == "bankedpages") {
        opts.banked_pages = parse_page_list(value);
    } else {
        throw std::runtime_error("unknown config key: " + raw_key);
    }
}

void load_config_file(options& opts, const std::filesystem::path& path) {
    const auto entries = xconfig::parse_simple_config_file(path);
    const auto base_dir = path.has_parent_path() ? path.parent_path()
                                                 : std::filesystem::path(".");

    memory_map_builder map_builder;
    for (const auto& entry : entries) {
        apply_config_entry(opts, map_builder, entry.key, entry.value, base_dir);
    }

    opts.memory_map = resolve_memory_map(map_builder);
    opts.loaded_config_path = std::filesystem::absolute(path);
    opts.config_path = path;
}

options parse_options(int argc, char* argv[]) {
    options opts;
    if (has_help_flag(argc, argv)) {
        opts.show_help = true;
        return opts;
    }

    if (const auto config_path = discover_config_path(argc, argv); config_path.has_value()) {
        load_config_file(opts, *config_path);
    }

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            opts.show_help = true;
            return opts;
        } else if (arg == "--version") {
            opts.show_version = true;
            return opts;
        } else if (arg == "-q" || arg == "--quiet") {
            opts.quiet = true;
        } else if (arg == "--no-quiet") {
            opts.quiet = false;
        } else if (arg == "--config") {
            if (++i >= argc) throw std::runtime_error("--config requires a path");
            opts.config_path = argv[i];
        } else if (arg == "--listen") {
            if (++i >= argc) throw std::runtime_error("--listen requires host:port");
            const auto [host, port] = split_host_port(argv[i]);
            opts.listen_host = host;
            opts.listen_port = port;
        } else if (arg == "--run") {
            opts.run_mode = true;
        } else if (arg == "--max-steps") {
            if (++i >= argc) throw std::runtime_error("--max-steps requires a value");
            opts.max_steps = static_cast<std::size_t>(parse_u32(argv[i]));
        } else if (arg == "--load-bin") {
            if (++i >= argc) throw std::runtime_error("--load-bin requires a path");
            opts.image_path = argv[i];
            opts.image = image_format::bin;
        } else if (arg == "--load-ihx") {
            if (++i >= argc) throw std::runtime_error("--load-ihx requires a path");
            opts.image_path = argv[i];
            opts.image = image_format::ihx;
        } else if (arg == "--load-elf") {
            if (++i >= argc) throw std::runtime_error("--load-elf requires a path");
            opts.image_path = argv[i];
            opts.image = image_format::elf;
        } else if (arg == "--origin") {
            if (++i >= argc) throw std::runtime_error("--origin requires a value");
            opts.origin = static_cast<uint16_t>(parse_u32(argv[i]));
        } else if (arg == "--pc") {
            if (++i >= argc) throw std::runtime_error("--pc requires a value");
            opts.pc = static_cast<uint16_t>(parse_u32(argv[i]));
        } else if (arg == "--sp") {
            if (++i >= argc) throw std::runtime_error("--sp requires a value");
            opts.sp = static_cast<uint16_t>(parse_u32(argv[i]));
        } else if (arg == "--emu-stdio") {
            opts.emu_stdio = true;
        } else if (arg == "--no-emu-stdio") {
            opts.emu_stdio = false;
        } else if (arg == "--emu-exit-status") {
            opts.emu_exit_status = true;
        } else if (arg == "--stdin-port") {
            if (++i >= argc) throw std::runtime_error("--stdin-port requires a value");
            opts.stdin_port = static_cast<uint16_t>(parse_u32(argv[i]));
        } else if (arg == "--stdin-status-port") {
            if (++i >= argc) throw std::runtime_error("--stdin-status-port requires a value");
            opts.stdin_status_port = static_cast<uint16_t>(parse_u32(argv[i]));
        } else if (arg == "--stdin-data-port") {
            if (++i >= argc) throw std::runtime_error("--stdin-data-port requires a value");
            opts.stdin_data_port = static_cast<uint16_t>(parse_u32(argv[i]));
        } else if (arg == "--stdout-port") {
            if (++i >= argc) throw std::runtime_error("--stdout-port requires a value");
            opts.stdout_port = static_cast<uint16_t>(parse_u32(argv[i]));
        } else if (arg == "--fs-root") {
            if (++i >= argc) throw std::runtime_error("--fs-root requires a path");
            opts.fs_root = argv[i];
        } else if (arg == "--shared-pages") {
            if (++i >= argc) throw std::runtime_error("--shared-pages requires a value");
            opts.shared_pages = parse_page_list(argv[i]);
        } else if (arg == "--banked-pages") {
            if (++i >= argc) throw std::runtime_error("--banked-pages requires a value");
            opts.banked_pages = parse_page_list(argv[i]);
        } else if (arg == "--bank-count") {
            if (++i >= argc) throw std::runtime_error("--bank-count requires a value");
            opts.bank_count = static_cast<uint16_t>(parse_u32(argv[i]));
        } else if (arg == "--bank-port") {
            if (++i >= argc) throw std::runtime_error("--bank-port requires a value");
            opts.bank_port = static_cast<uint16_t>(parse_u32(argv[i]));
        } else {
            throw std::runtime_error("unknown option: " + arg);
        }
    }
    return opts;
}

std::vector<uint8_t> complement_pages(const std::vector<uint8_t>& pages) {
    std::array<bool, xemu::banked_memory_config::page_count> seen{};
    for (const auto page : pages) {
        if (page >= xemu::banked_memory_config::page_count) {
            throw std::runtime_error("page index out of range: " + std::to_string(page));
        }
        if (seen[page]) {
            throw std::runtime_error("duplicate page index: " + std::to_string(page));
        }
        seen[page] = true;
    }

    std::vector<uint8_t> result;
    for (uint8_t page = 0; page < xemu::banked_memory_config::page_count; ++page) {
        if (!seen[page]) {
            result.push_back(page);
        }
    }
    return result;
}

std::optional<xemu::banked_memory_config> resolve_compat_banked_memory_config(
    const options& opts)
{
    const bool any_banked_option =
        opts.bank_port.has_value() || opts.bank_count.has_value()
        || opts.shared_pages.has_value() || opts.banked_pages.has_value();
    if (!any_banked_option) {
        return std::nullopt;
    }

    if (!opts.bank_count.has_value()) {
        throw std::runtime_error("compatibility banking requires --bank-count");
    }
    if (!opts.bank_port.has_value()) {
        throw std::runtime_error("compatibility banking requires --bank-port");
    }

    std::vector<uint8_t> shared_pages;
    std::vector<uint8_t> banked_pages;
    if (opts.shared_pages.has_value() && opts.banked_pages.has_value()) {
        shared_pages = *opts.shared_pages;
        banked_pages = *opts.banked_pages;
    } else if (opts.shared_pages.has_value()) {
        shared_pages = *opts.shared_pages;
        banked_pages = complement_pages(shared_pages);
    } else if (opts.banked_pages.has_value()) {
        banked_pages = *opts.banked_pages;
        shared_pages = complement_pages(banked_pages);
    } else {
        throw std::runtime_error(
            "compatibility banking requires --shared-pages, --banked-pages, or both");
    }

    return xemu::banked_memory_config{
        std::move(shared_pages),
        std::move(banked_pages),
        opts.bank_count.value()
    };
}

void print_memory_map_summary(const xemu::memory_map_config& config) {
    std::cout << "memory map: " << config.stores.size() << " stores, "
              << config.windows.size() << " windows, "
              << config.selectors.size() << " selectors, "
              << config.port_rules.size() << " port rules\n";
}

void configure_machine(xemu::machine& emu, options& opts) {
    const bool has_simple_stdin = opts.stdin_port.has_value();
    const bool has_split_stdin =
        opts.stdin_status_port.has_value() || opts.stdin_data_port.has_value();

    if (opts.memory_map.has_value()) {
        const bool any_compat_banking =
            opts.bank_port.has_value() || opts.bank_count.has_value()
            || opts.shared_pages.has_value() || opts.banked_pages.has_value();
        if (any_compat_banking) {
            throw std::runtime_error(
                "cannot mix generic memory-map config with compatibility banking options");
        }
        emu.configure_memory_map(*opts.memory_map);
    } else if (const auto compat_banked = resolve_compat_banked_memory_config(opts);
               compat_banked.has_value()) {
        emu.configure_banked_memory(*compat_banked);
        emu.bind_bank_port(opts.bank_port.value());
    }

    if (opts.image_path.has_value()) {
        if (opts.image == image_format::ihx) {
            emu.load_ihx(opts.image_path.value());
        } else if (opts.image == image_format::elf) {
            opts.image_entry = emu.load_elf(opts.image_path.value());
        } else {
            emu.load_binary(opts.image_path.value(), opts.origin);
        }
    }
    if (opts.fs_root.has_value())
        emu.bind_host_filesystem(*opts.fs_root);

    if (has_simple_stdin && has_split_stdin) {
        throw std::runtime_error(
            "cannot combine --stdin-port with --stdin-status-port/--stdin-data-port");
    }
    if (has_split_stdin &&
        (!opts.stdin_status_port.has_value() || !opts.stdin_data_port.has_value())) {
        throw std::runtime_error(
            "--stdin-status-port and --stdin-data-port must be provided together");
    }

    if (opts.emu_stdio) {
        if (has_simple_stdin) {
            throw std::runtime_error("cannot combine --emu-stdio with --stdin-port");
        }
        emu.bind_stdin_status_data(
            opts.stdin_status_port.value_or(xemu::machine::emu_stdin_status_port),
            opts.stdin_data_port.value_or(xemu::machine::emu_stdin_data_port),
            std::cin);
        emu.bind_stdout(
            opts.stdout_port.value_or(xemu::machine::emu_stdout_port),
            std::cout);
    } else if (opts.stdin_status_port.has_value()) {
        emu.bind_stdin_status_data(
            opts.stdin_status_port.value(),
            opts.stdin_data_port.value(),
            std::cin);
    } else if (opts.stdin_port.has_value()) {
        emu.bind_stdin(opts.stdin_port.value(), std::cin);
    }

    if (!opts.emu_stdio && opts.stdout_port.has_value()) {
        emu.bind_stdout(opts.stdout_port.value(), std::cout);
    }

    emu.set_pc(opts.pc.has_value()
        ? opts.pc.value()
        : opts.image_entry.value_or(opts.origin));
    emu.set_sp(opts.sp);
}

int run_program(xemu::machine& emu, const options& opts) {
    const auto stop = emu.continue_execution(opts.max_steps);
    if (!opts.quiet) {
        switch (stop.reason) {
        case xemu::stop_reason::halted:
            std::cerr << "xemu: halted at 0x" << std::hex << stop.pc << std::dec << "\n";
            break;
        case xemu::stop_reason::breakpoint:
            std::cerr << "xemu: breakpoint at 0x" << std::hex << stop.pc << std::dec << "\n";
            break;
        case xemu::stop_reason::step_limit:
            std::cerr << "xemu: step limit reached at 0x" << std::hex << stop.pc
                      << std::dec << "\n";
            break;
        case xemu::stop_reason::fault:
            std::cerr << "xemu: fault at 0x" << std::hex << stop.pc << std::dec;
            if (!stop.message.empty()) {
                std::cerr << " - " << stop.message;
            }
            std::cerr << "\n";
            break;
        case xemu::stop_reason::stepped:
        case xemu::stop_reason::none:
            break;
        }
    }

    if (stop.reason != xemu::stop_reason::halted)
        return 2;

    if (opts.emu_exit_status) {
        constexpr uint16_t result_addr = 0xff00;
        constexpr uint16_t done_addr = 0xff02;
        constexpr uint8_t done_magic = 0xa5;

        if (emu.read_byte(done_addr) != done_magic) {
            if (!opts.quiet)
                std::cerr << "xemu: platform=emu exit mailbox was not completed\n";
            return 2;
        }

        const uint16_t raw =
            static_cast<uint16_t>(emu.read_byte(result_addr))
            | static_cast<uint16_t>(
                static_cast<uint16_t>(emu.read_byte(result_addr + 1)) << 8);
        const int status = static_cast<int16_t>(raw);
        if (status == 0)
            return 0;
        return status > 0 && status <= 255 ? status : 1;
    }

    return 0;
}

int serve_debugger(xemu::machine& emu, const options& opts) {
    xemu::rsp_target_adapter target(emu);
    rsp::server server;
    server.listen(opts.listen_host, opts.listen_port);

    if (!opts.quiet) {
        std::cout << "xemu listening on "
                  << opts.listen_host << ":" << opts.listen_port << "\n";
        if (opts.loaded_config_path.has_value()) {
            std::cout << "loaded config " << *opts.loaded_config_path << "\n";
        }
        if (opts.image_path.has_value()) {
            std::cout << "loaded " << opts.image_path.value();
            if (opts.image == image_format::bin)
                std::cout << " at 0x" << std::hex << opts.origin << std::dec;
            else if (opts.image == image_format::elf && opts.image_entry.has_value())
                std::cout << " entry 0x" << std::hex << *opts.image_entry << std::dec;
            std::cout << "\n";
        }
        if (opts.memory_map.has_value()) {
            print_memory_map_summary(*opts.memory_map);
        } else if (const auto compat_banked = resolve_compat_banked_memory_config(opts);
                   compat_banked.has_value()) {
            std::cout << "compatibility banking: " << compat_banked->bank_count
                      << " banks via port 0x" << std::hex << opts.bank_port.value()
                      << std::dec << "\n";
        }
        if (opts.emu_stdio) {
            std::cout << "platform=emu stdin status mapped to port 0x" << std::hex
                      << opts.stdin_status_port.value_or(
                             xemu::machine::emu_stdin_status_port)
                      << ", data to 0x"
                      << opts.stdin_data_port.value_or(xemu::machine::emu_stdin_data_port)
                      << ", stdout to 0x"
                      << opts.stdout_port.value_or(xemu::machine::emu_stdout_port)
                      << std::dec << "\n";
        } else if (opts.stdin_port.has_value()) {
            std::cout << "stdin mapped to port 0x" << std::hex
                      << opts.stdin_port.value() << std::dec << "\n";
        } else if (opts.stdin_status_port.has_value()) {
            std::cout << "stdin status mapped to port 0x" << std::hex
                      << opts.stdin_status_port.value()
                      << ", data to 0x" << opts.stdin_data_port.value()
                      << std::dec << "\n";
        }
        if (!opts.emu_stdio && opts.stdout_port.has_value())
            std::cout << "stdout mapped to port 0x" << std::hex
                      << opts.stdout_port.value() << std::dec << "\n";
        if (opts.fs_root.has_value())
            std::cout << "platform=emu filesystem rooted at "
                      << std::filesystem::absolute(*opts.fs_root) << "\n";
        std::cout << "connect with: target remote "
                  << opts.listen_host << ":" << opts.listen_port << "\n";
    }

    while (server.is_listening()) {
        try {
            server.serve(target);
            if (!opts.quiet)
                std::cout << "xemu client disconnected, waiting for reconnect\n";
        } catch (const rsp::error& e) {
            if (!server.is_listening())
                break;
            if (std::string(e.what()).find("connection closed") != std::string::npos) {
                if (!opts.quiet)
                    std::cout << "xemu client disconnected\n";
                continue;
            }
            throw;
        }
    }
    return 0;
}

} // namespace

int main(int argc, char* argv[]) {
    try {
        auto opts = parse_options(argc, argv);
        if (opts.show_help) {
            print_help();
            return 0;
        }
        if (opts.show_version) {
            std::cout << "xemu " << XTOOLS_VERSION
                      << " (X Tools Emulator for Z80)\n";
            return 0;
        }

        xemu::machine emu;
        configure_machine(emu, opts);

        if (opts.run_mode)
            return run_program(emu, opts);
        return serve_debugger(emu, opts);
    } catch (const std::exception& e) {
        std::cerr << "xemu: " << e.what() << "\n";
        return 1;
    }
}
