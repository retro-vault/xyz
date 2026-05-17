#include "appmake/cli.h"

#include <format>
#include <iostream>
#include <stdexcept>

#include <microdrive/microdrive.h>
#include "appmake/analysis.h"
#include "appmake/app_image.h"
#include "appmake/basic.h"
#include "appmake/tape.h"
#include "appmake/util.h"

namespace appmake {

namespace {

std::string sanitize_app_base(std::string name) {
    std::string out;
    out.reserve(name.size());

    for (const char ch : name) {
        if ((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9')) {
            out.push_back(ch);
            continue;
        }
        if (ch >= 'A' && ch <= 'Z') {
            out.push_back(static_cast<char>(ch - 'A' + 'a'));
        }
    }

    if (out.empty()) {
        out = "legacy";
    }
    if (out.size() > 6) {
        out.resize(6);
    }
    return out;
}

std::string make_app_filename(const fs::path& input, const cli_options& options) {
    if (options.app_name) {
        std::string name = lower_copy(*options.app_name);
        if (name.find('.') == std::string::npos) {
            name += ".app";
        }
        if (name.size() > microdrive::k_name_len) {
            throw std::runtime_error("microdrive app name must be 10 characters or fewer");
        }
        return name;
    }

    return sanitize_app_base(input.stem().string()) + ".app";
}

const tap_file& pick_program_file(
    const std::vector<tap_file>& files,
    const cli_options& options,
    bool has_usr_addr,
    uint16_t usr_addr
) {
    const std::optional<std::string> wanted =
        options.name ? std::optional<std::string>(upper_copy(*options.name)) : std::nullopt;

    if (wanted) {
        for (const auto& file : files) {
            if (file.header.type == 0x03 && upper_copy(file.header.name) == *wanted) {
                return file;
            }
        }
        throw std::runtime_error(std::format("no matching CODE block found on tape: {}", *options.name));
    }

    if (has_usr_addr) {
        for (const auto& file : files) {
            if (file.header.type != 0x03) {
                continue;
            }

            const uint32_t begin = file.header.param1;
            const uint32_t end = begin + static_cast<uint32_t>(file.data.size());
            if (usr_addr >= begin && usr_addr < end) {
                return file;
            }
        }
    }

    const tap_file* best = nullptr;
    for (const auto& file : files) {
        if (file.header.type != 0x03) {
            continue;
        }

        const std::string role = zx_code_role(file.header.param1, file.header.data_len);
        const std::string best_role = best
            ? zx_code_role(best->header.param1, best->header.data_len)
            : "";
        const bool better_role = !best
            || (role == "program_code" && best_role != "program_code");
        const bool better_size = !best || file.data.size() > best->data.size();

        if (!best || better_role || (role == best_role && better_size)) {
            best = &file;
        }
    }

    if (!best) {
        throw std::runtime_error("no CODE block found on tape");
    }

    return *best;
}

}  // namespace

cli_options parse_options(const std::vector<std::string_view>& args, std::size_t start_index) {
    cli_options options;

    for (std::size_t i = start_index; i < args.size(); ++i) {
        const std::string_view arg = args[i];

        auto need_value = [&](std::string_view opt) -> std::string_view {
            if (i + 1 >= args.size()) {
                throw std::runtime_error(std::format("missing value for {}", opt));
            }
            return args[++i];
        };

        if (arg == "--name") {
            options.name = std::string(need_value(arg));
        } else if (arg == "--app") {
            options.app_name = std::string(need_value(arg));
        } else if (arg == "--cart") {
            options.cart_name = std::string(need_value(arg));
        } else if (arg == "--load") {
            options.load_addr = parse_u16(need_value(arg));
        } else if (arg == "--start") {
            options.entry_addr = parse_u16(need_value(arg));
        } else if (arg == "--sp") {
            options.stack_ptr = parse_u16(need_value(arg));
        } else {
            throw std::runtime_error(std::format("unknown option: {}", arg));
        }
    }

    return options;
}

void print_list(const fs::path& tape_path, const std::vector<tape_list_entry>& entries) {
    std::cout << std::format("file: {}\n", tape_path.string());
    std::cout << std::format("{:>3}  {:<10} {:<16} {:<18} {:<16} {:>8}  {}\n",
                             "#", "source", "kind", "role", "name", "bytes", "details");
    std::cout << std::string(96, '-') << "\n";

    for (const auto& entry : entries) {
        const std::string size_text = entry.size
            ? std::format("{}", *entry.size)
            : "";

        std::cout << std::format("{:>3}  {:<10} {:<16} {:<18} {:<16} {:>8}  {}\n",
                                 entry.index,
                                 entry.source,
                                 entry.kind,
                                 entry.role,
                                 entry.name,
                                 size_text,
                                 entry.details);
    }

    if (entries.empty()) {
        std::cout << "no entries.\n";
    }
}

void cmd_list(const fs::path& tape_path) {
    const std::string ext = lower_copy(tape_path.extension().string());

    if (ext == ".tap") {
        print_list(tape_path, parse_tap_list(tape_path));
        return;
    }

    if (ext == ".tzx") {
        print_list(tape_path, parse_tzx_list(tape_path));
        return;
    }

    throw std::runtime_error("list currently supports .tap and .tzx files");
}

void cmd_analyze(const fs::path& input) {
    const std::string ext = lower_copy(input.extension().string());
    if (ext == ".tap") {
        print_analysis_report(input, analyze_tap(input));
        return;
    }
    if (ext == ".tzx") {
        print_analysis_report(input, analyze_tzx(input));
        return;
    }
    throw std::runtime_error("analyze currently supports .tap and .tzx files");
}

void cmd_make(const fs::path& input, const fs::path& mdr_path, const cli_options& options) {
    const auto files = parse_tape_files(input);

    std::optional<basic_program> basic;
    try {
        basic = parse_basic_from_files(files);
    } catch (const std::runtime_error&) {
    }

    const bool has_usr_addr = basic && basic->usr_addr.has_value();
    const uint16_t usr_addr = has_usr_addr ? *basic->usr_addr : 0;
    const tap_file& program = pick_program_file(files, options, has_usr_addr, usr_addr);
    const uint16_t load_addr = options.load_addr.value_or(program.header.param1);
    const uint16_t entry_addr = options.entry_addr.value_or(
        basic && basic->usr_addr ? *basic->usr_addr : load_addr
    );
    const uint16_t stack_ptr = options.stack_ptr.value_or(
        basic && basic->clear_addr ? *basic->clear_addr : 0
    );

    validate_range(load_addr, program.data.size(), entry_addr);

    app_header header;
    header.kind = k_kind_tape_code;
    header.flags = static_cast<uint8_t>(k_flag_legacy_zx | k_flag_absolute_load);
    header.load_addr = load_addr;
    header.entry_addr = entry_addr;
    header.payload_size = static_cast<uint16_t>(program.data.size());
    header.stack_ptr = stack_ptr;
    header.tape_flag = program.tape_flag;
    header.tape_checksum = program.tape_checksum;

    const std::vector<uint8_t> app = build_app(header, {}, program.data);
    const std::string app_name = make_app_filename(input, options);
    const std::string cart_name = options.cart_name.value_or("YOS");

    microdrive::image_t image = fs::exists(mdr_path)
        ? microdrive::image_t::load(mdr_path)
        : microdrive::image_t::create_blank(cart_name);
    image.put(app_name, app);
    image.save(mdr_path);

    std::cout << std::format("make: {} -> {} \"{}\" block=\"{}\" load=0x{:04x} start=0x{:04x}",
                             input.string(),
                             mdr_path.string(),
                             app_name,
                             program.header.name,
                             load_addr,
                             entry_addr);
    if (stack_ptr != 0) {
        std::cout << std::format(" sp=0x{:04x}", stack_ptr);
    }
    std::cout << std::format(" tape_xor=0x{:02x} bytes={}\n",
                             header.tape_checksum,
                             app.size());
}

void cmd_tap(const fs::path& input, const fs::path& output, const cli_options& options) {
    const tap_code_block tap = parse_tap_code(input, options.name);
    const uint16_t load_addr = options.load_addr.value_or(tap.load_addr);
    const uint16_t entry_addr = options.entry_addr.value_or(load_addr);

    validate_range(load_addr, tap.data.size(), entry_addr);

    app_header header;
    header.kind = k_kind_tape_code;
    header.flags = static_cast<uint8_t>(k_flag_legacy_zx | k_flag_absolute_load);
    header.load_addr = load_addr;
    header.entry_addr = entry_addr;
    header.payload_size = static_cast<uint16_t>(tap.data.size());

    write_file(output, build_app(header, {}, tap.data));

    std::cout << std::format("tap: {} -> {} name=\"{}\" load=0x{:04x} start=0x{:04x} bytes={}\n",
                             input.string(),
                             output.string(),
                             tap.name,
                             load_addr,
                             entry_addr,
                             tap.data.size());
}

void cmd_sna(const fs::path& input, const fs::path& output, const cli_options& options) {
    if (!options.load_addr) {
        throw std::runtime_error("--load is required for .sna input");
    }

    const snapshot_48 sna = parse_sna48(input);
    const uint16_t load_addr = *options.load_addr;
    if (load_addr < 0x4000) {
        throw std::runtime_error("snapshot --load must be within 48K RAM (>= 0x4000)");
    }

    const uint16_t entry_addr = options.entry_addr.value_or(sna.pc);
    const uint16_t stack_ptr = options.stack_ptr.value_or(sna.sp);

    if (entry_addr < load_addr) {
        throw std::runtime_error("snapshot entry address falls below preserved RAM");
    }
    if (stack_ptr < load_addr) {
        throw std::runtime_error("snapshot stack pointer falls below preserved RAM");
    }

    const std::size_t ram_off = static_cast<std::size_t>(load_addr - 0x4000);
    if (ram_off >= sna.ram.size()) {
        throw std::runtime_error("snapshot --load is past the end of RAM");
    }

    std::vector<uint8_t> payload(sna.ram.begin() + static_cast<std::ptrdiff_t>(ram_off), sna.ram.end());
    validate_range(load_addr, payload.size(), entry_addr);

    const std::vector<uint8_t> state = build_snapshot_state(sna, stack_ptr);

    app_header header;
    header.kind = k_kind_snapshot_48;
    header.flags = static_cast<uint8_t>(k_flag_legacy_zx | k_flag_has_state | k_flag_absolute_load);
    header.load_addr = load_addr;
    header.entry_addr = entry_addr;
    header.payload_size = static_cast<uint16_t>(payload.size());
    header.state_size = static_cast<uint16_t>(state.size());
    header.stack_ptr = stack_ptr;

    write_file(output, build_app(header, state, payload));

    std::cout << std::format("sna: {} -> {} load=0x{:04x} start=0x{:04x} sp=0x{:04x} bytes={}\n",
                             input.string(),
                             output.string(),
                             load_addr,
                             entry_addr,
                             stack_ptr,
                             payload.size());
}

void print_usage() {
    std::cerr
        << "usage:\n"
        << "  appmake list <tape.tap|tape.tzx>\n"
        << "  appmake analyze <input.tap>\n"
        << "  appmake make <input.tap|input.tzx> <cart.mdr> [--app NAME] [--name CODE] [--cart CART] [--load ADDR] [--start ADDR] [--sp ADDR]\n"
        << "  appmake tap  <input.tap> <output.app> [--name NAME] [--load ADDR] [--start ADDR]\n"
        << "  appmake sna  <input.sna> <output.app> --load ADDR [--start ADDR] [--sp ADDR]\n";
}

}  // namespace appmake
