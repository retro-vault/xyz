#include <charconv>
#include <iostream>
#include <limits>
#include <string_view>

#include <xprog/cli.h>
#include <xprog/errors.h>

namespace xprog {
namespace {

std::uint32_t number(const std::string& text, std::uint32_t maximum,
                     const std::string& option)
{
    if (text.empty())
        throw usage_error(option + " requires a number");
    std::string_view value(text);
    int base = 10;
    if (value.size() > 2 && value[0] == '0'
        && (value[1] == 'x' || value[1] == 'X')) {
        value.remove_prefix(2);
        base = 16;
    }
    std::uint32_t result = 0;
    const auto converted = std::from_chars(
        value.data(), value.data() + value.size(), result, base);
    if (value.empty() || converted.ec != std::errc()
        || converted.ptr != value.data() + value.size() || result > maximum) {
        throw usage_error("invalid value for " + option + ": " + text);
    }
    return result;
}

std::string take_value(int& i, int argc, char* argv[], const std::string& opt)
{
    if (++i >= argc)
        throw usage_error(opt + " requires an argument");
    return argv[i];
}

} // namespace

cli_options cli::parse(int argc, char* argv[])
{
    cli_options options;
    if (argc < 2) {
        options.show_help = true;
        return options;
    }

    const auto select_command = [&options](command_kind command,
                                            const std::string& option) {
        if (options.command != command_kind::none
            && options.command != command) {
            throw usage_error(option + " conflicts with another mode switch");
        }
        options.command = command;
    };

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            options.show_help = true;
            return options;
        } else if (arg == "--version") {
            options.show_version = true;
            return options;
        } else if (arg == "-p" || arg == "--process") {
            select_command(command_kind::process, arg);
        } else if (arg == "-s" || arg == "--service") {
            select_command(command_kind::service, arg);
        } else if (arg == "-i" || arg == "--inspect") {
            select_command(command_kind::inspect, arg);
        } else if (arg == "--tap") {
            select_command(command_kind::tap, arg);
        } else if (arg == "--tzx") {
            select_command(command_kind::tzx, arg);
        } else if (arg == "-o") {
            options.output_file = take_value(i, argc, argv, "-o");
        } else if (arg.rfind("-o", 0) == 0 && arg.size() > 2) {
            options.output_file = arg.substr(2);
        } else if (arg == "-n" || arg == "--name") {
            options.name = take_value(i, argc, argv, arg);
        } else if (arg == "--id") {
            options.image_id = number(take_value(i, argc, argv, arg),
                                      UINT32_MAX, arg);
        } else if (arg == "--abi") {
            options.abi_version = static_cast<std::uint8_t>(number(
                take_value(i, argc, argv, arg), UINT8_MAX, arg));
        } else if (arg == "--min-os") {
            options.minimum_os_version = static_cast<std::uint16_t>(number(
                take_value(i, argc, argv, arg), UINT16_MAX, arg));
        } else if (arg == "--load-address") {
            options.load_address = static_cast<std::uint16_t>(number(
                take_value(i, argc, argv, arg), UINT16_MAX, arg));
        } else if (arg == "--fixed-load") {
            options.require_fixed_load = true;
        } else if (arg == "--entry") {
            options.entry_point = static_cast<std::uint16_t>(number(
                take_value(i, argc, argv, arg), UINT16_MAX, arg));
        } else if (arg == "--stack-size") {
            options.stack_size = static_cast<std::uint16_t>(number(
                take_value(i, argc, argv, arg), UINT16_MAX, arg));
        } else if (arg == "--export") {
            options.exports.push_back(static_cast<std::uint16_t>(number(
                take_value(i, argc, argv, arg), UINT16_MAX, arg)));
        } else if (!arg.empty() && arg[0] == '-') {
            throw usage_error("unknown option: " + arg);
        } else if (options.input_file.empty()) {
            options.input_file = arg;
        } else {
            throw usage_error("too many positional arguments");
        }
    }

    if (options.show_help)
        return options;
    if (options.input_file.empty())
        throw usage_error("input file is required");
    if (options.command == command_kind::none)
        throw usage_error("select --process, --service, --inspect, --tap, or --tzx");
    if (options.command == command_kind::inspect) {
        if (!options.output_file.empty())
            throw usage_error("inspect does not accept -o");
        return options;
    }
    if (options.output_file.empty()) {
        options.output_file = options.input_file;
        const char* extension = options.command == command_kind::process
            ? ".prc" : options.command == command_kind::service
            ? ".svc" : options.command == command_kind::tap ? ".tap" : ".tzx";
        options.output_file.replace_extension(extension);
    }
    if (options.name.empty())
        options.name = options.input_file.stem().string();
    const bool tape = options.command == command_kind::tap
                   || options.command == command_kind::tzx;
    const std::size_t max_name = tape ? 10 : 15;
    if (options.name.empty() || options.name.size() > max_name)
        throw usage_error("image name must contain 1 to "
                          + std::to_string(max_name) + " bytes");
    if (tape) {
        if (options.load_address == 0)
            options.load_address = 0x5ccb;
        if (!options.entry_point.has_value())
            options.entry_point = options.load_address;
        if (options.stack_size.has_value() || !options.exports.empty()
            || options.require_fixed_load || options.image_id.has_value()
            || options.abi_version != 1 || options.minimum_os_version != 0) {
            throw usage_error("process/service options are not valid for tape output");
        }
        return options;
    }
    if (options.command == command_kind::process) {
        if (!options.stack_size.has_value() || options.stack_size.value() == 0)
            throw usage_error("process requires --stack-size with a nonzero value");
        if (!options.exports.empty())
            throw usage_error("--export is only valid for a service");
    } else {
        if (options.stack_size.has_value())
            throw usage_error("--stack-size is only valid for a process");
        if (options.exports.empty())
            throw usage_error("service requires at least one --export");
    }
    return options;
}

void cli::print_usage(const char* argv0)
{
    std::cerr
        << "Usage: " << argv0 << " [options] <input>\n\n"
        << "X Tools Program Packager (xprog) — XPRG and Spectrum tape images\n\n"
        << "mode (one required):\n"
        << "  -p, --process             Create a .prc process image\n"
        << "  -s, --service             Create a .svc service image\n"
        << "  -i, --inspect             Validate and describe an existing image\n\n"
        << "  --tap                     Wrap a flat binary in an auto-running .tap\n"
        << "  --tzx                     Wrap a flat binary in an auto-running .tzx\n\n"
        << "options:\n"
        << "  -o <file>                 Output image (default: .prc or .svc)\n"
        << "  -n, --name <name>         Image name (10 bytes for tape, 15 for XPRG)\n"
        << "  --id <n>                  Stable 32-bit image/service identifier\n"
        << "  --abi <n>                 Provided ABI version (default: 1)\n"
        << "  --min-os <n>              Minimum OS ABI version (default: 0)\n"
        << "  --load-address <n>        Load address (tape default: 0x5CCB)\n"
        << "  --fixed-load              Require the preferred address\n"
        << "  --entry <n>               XL entry override or absolute tape entry\n"
        << "  --version                 Show version\n"
        << "  -h, --help                Show this help\n\n"
        << "process options:\n"
        << "  --stack-size <n>          Required stack size in bytes\n\n"
        << "service options:\n"
        << "  --export <offset>         Add a JP slot targeting an XL code offset;\n"
        << "                            repeat in stable ABI slot order\n\n"
        << "Numbers may be decimal or hexadecimal (0x...).\n";
}

} // namespace xprog
