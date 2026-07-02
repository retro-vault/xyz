//
// main.cpp -- xopt standalone optimizer.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//

#include <xopt/xopt.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

#ifndef XTOOLS_VERSION
#define XTOOLS_VERSION "0.1.0"
#endif

struct cli_options {
    xopt::optimizer_options optimizer;
    std::vector<std::string> inputs;
    std::string output;
    std::string out_dir;
    bool in_place = false;
    bool force_stdout = false;
    bool stats = false;
    bool reg_coverage = false;
};

[[noreturn]] void print_usage_and_exit(const char *prog, int code) {
    std::cerr <<
        "Usage: " << prog << " [options] <input.s>...\n"
        "\n"
        "X Tools Optimizer (xopt) — Z80 assembly optimizer\n"
        "\n"
        "options:\n"
        "  -O0|-O2|-Os|-Of|-O3      Select optimization level (default: -O2)\n"
        "  -o <file>                Write one optimized output file\n"
        "  --out-dir <dir>          Write one optimized file per input under dir\n"
        "  --in-place               Replace each input with optimized output\n"
        "  --stdout                 Write optimized output to stdout\n"
        "  --cross-file             Optimize all inputs as one combined unit\n"
        "  --stats                  Print byte/cycle savings table only\n"
        "  --reg-coverage           Print register coverage/pressure analysis\n"
        "  --version                Show version\n"
        "  -h, --help               Show this help\n"
        "\n"
        "examples:\n"
        "  " << prog << " -O3 --out-dir optimized *.s\n"
        "  " << prog << " -O3 --cross-file *.s -o combined.s\n"
        "  " << prog << " --stats -O3 *.s\n"
        "  " << prog << " --reg-coverage *.s\n";
    std::exit(code);
}

bool starts_with(const std::string &text, const std::string &prefix) {
    return text.size() >= prefix.size() &&
           text.compare(0, prefix.size(), prefix) == 0;
}

xopt::optimization_level parse_level(const std::string &arg) {
    if (arg == "-O0")
        return xopt::optimization_level::none;
    if (arg == "-O1")
        throw std::runtime_error(
            "xopt does not support -O1; use -O2, -Os, -Of, -O3, or -O0");
    if (arg == "-O2")
        return xopt::optimization_level::o2;
    if (arg == "-Os")
        return xopt::optimization_level::os;
    if (arg == "-Of")
        return xopt::optimization_level::of;
    if (arg == "-O3")
        return xopt::optimization_level::o3;
    throw std::runtime_error("unknown optimization level: " + arg);
}

cli_options parse_args(int argc, char **argv) {
    cli_options opts;
    const char *prog = argc > 0 ? argv[0] : "xopt";

    if (argc < 2)
        print_usage_and_exit(prog, 1);

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help")
            print_usage_and_exit(prog, 0);
        if (arg == "--version") {
            std::cout << "xopt " << XTOOLS_VERSION
                      << " (X Tools Optimizer for Z80)\n";
            std::exit(0);
        }
        if (arg == "-o") {
            if (++i >= argc)
                throw std::runtime_error("-o requires a file");
            opts.output = argv[i];
        } else if (arg == "--out-dir") {
            if (++i >= argc)
                throw std::runtime_error("--out-dir requires a directory");
            opts.out_dir = argv[i];
        } else if (arg == "--in-place") {
            opts.in_place = true;
        } else if (arg == "--stdout") {
            opts.force_stdout = true;
        } else if (arg == "--cross-file") {
            opts.optimizer.cross_file = true;
        } else if (arg == "--stats") {
            opts.stats = true;
        } else if (arg == "--reg-coverage") {
            opts.reg_coverage = true;
        } else if (starts_with(arg, "-O")) {
            opts.optimizer.level = parse_level(arg);
        } else if (!arg.empty() && arg[0] == '-') {
            throw std::runtime_error("unknown option: " + arg);
        } else {
            opts.inputs.push_back(arg);
        }
    }

    if (opts.inputs.empty())
        print_usage_and_exit(prog, 1);

    const int sinks = (opts.output.empty() ? 0 : 1) +
                      (opts.out_dir.empty() ? 0 : 1) +
                      (opts.in_place ? 1 : 0) +
                      (opts.force_stdout ? 1 : 0);
    if (sinks > 1)
        throw std::runtime_error("choose only one output mode");

    if (opts.stats && sinks > 0)
        throw std::runtime_error("--stats does not write optimized output files");
    if (opts.reg_coverage && sinks > 0)
        throw std::runtime_error("--reg-coverage does not write optimized output files");
    if (opts.stats && opts.reg_coverage)
        throw std::runtime_error("choose only one analysis mode");
    if (!opts.output.empty() && opts.inputs.size() != 1 &&
        !opts.optimizer.cross_file) {
        throw std::runtime_error("-o with multiple inputs requires --cross-file");
    }
    if (opts.optimizer.cross_file && (!opts.out_dir.empty() || opts.in_place))
        throw std::runtime_error("--cross-file writes one stream; use -o or --stdout");

    return opts;
}

std::string read_text_file(const std::string &path) {
    std::ifstream in(path);
    if (!in)
        throw std::runtime_error("cannot open input: " + path);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

void write_text_file(const std::string &path, const std::string &text) {
    std::ofstream out(path);
    if (!out)
        throw std::runtime_error("cannot open output: " + path);
    out << text;
    if (!out)
        throw std::runtime_error("failed to write output: " + path);
}

std::string combine_inputs(const std::vector<std::string> &inputs) {
    std::ostringstream combined;
    for (const auto &path : inputs) {
        combined << "; xopt: begin " << path << "\n";
        combined << read_text_file(path);
        combined << "\n; xopt: end " << path << "\n";
    }
    return combined.str();
}

std::string out_dir_path(const std::string &dir, const std::string &input) {
    std::filesystem::path out = std::filesystem::path(dir) /
                                std::filesystem::path(input).filename();
    return out.string();
}

void print_stats_header(size_t name_width) {
    std::cout << std::left << std::setw((int)name_width) << "file"
              << std::right
              << std::setw(10) << "bytes"
              << std::setw(10) << "opt"
              << std::setw(10) << "saved"
              << std::setw(12) << "cycles"
              << std::setw(12) << "opt"
              << std::setw(12) << "saved"
              << std::setw(8) << "unk"
              << "\n";
    std::cout << std::string(name_width + 74, '-') << "\n";
}

void print_stats_row(size_t name_width, const std::string &name,
                     const xopt::optimization_stats &stats) {
    const long unknown = stats.before.unknown_instructions +
                         stats.after.unknown_instructions;
    std::cout << std::left << std::setw((int)name_width) << name
              << std::right
              << std::setw(10) << stats.before.bytes
              << std::setw(10) << stats.after.bytes
              << std::setw(10) << stats.bytes_saved()
              << std::setw(12) << stats.before.cycles
              << std::setw(12) << stats.after.cycles
              << std::setw(12) << stats.cycles_saved()
              << std::setw(8) << unknown
              << "\n";
}

size_t stats_name_width(const std::vector<std::string> &inputs,
                        bool cross_file) {
    size_t width = cross_file ? std::string("<cross-file>").size() : 4;
    for (const auto &input : inputs)
        width = std::max(width, input.size());
    return std::max<size_t>(width + 2, 24);
}

int run_stats(const cli_options &opts) {
    const size_t name_width = stats_name_width(opts.inputs,
                                               opts.optimizer.cross_file);
    print_stats_header(name_width);

    xopt::optimization_stats total;
    if (opts.optimizer.cross_file) {
        const xopt::optimization_stats stats =
            xopt::analyze_assembly_optimization(combine_inputs(opts.inputs),
                                                 opts.optimizer);
        print_stats_row(name_width, "<cross-file>", stats);
        return 0;
    }

    for (const auto &input : opts.inputs) {
        const xopt::optimization_stats stats =
            xopt::analyze_assembly_optimization(read_text_file(input),
                                                 opts.optimizer);
        print_stats_row(name_width, input, stats);
        total.before.bytes += stats.before.bytes;
        total.before.cycles += stats.before.cycles;
        total.before.unknown_instructions += stats.before.unknown_instructions;
        total.after.bytes += stats.after.bytes;
        total.after.cycles += stats.after.cycles;
        total.after.unknown_instructions += stats.after.unknown_instructions;
    }

    if (opts.inputs.size() > 1) {
        std::cout << std::string(name_width + 74, '-') << "\n";
        print_stats_row(name_width, "total", total);
    }

    return 0;
}

std::string reg_density_list(
        const std::vector<xopt::register_touch> &registers,
        long instruction_count,
        bool hot) {
    auto is_alt_register = [](const std::string &reg) {
        return !reg.empty() && reg.back() == '\'';
    };

    std::vector<xopt::register_touch> regs = registers;
    std::sort(regs.begin(), regs.end(), [hot](const auto &a, const auto &b) {
        if (a.touches != b.touches)
            return hot ? a.touches > b.touches : a.touches < b.touches;
        return a.reg < b.reg;
    });

    std::ostringstream out;
    int emitted = 0;
    for (const auto &reg : regs) {
        if (hot && reg.touches == 0)
            continue;
        if (!hot && reg.touches == 0 && is_alt_register(reg.reg))
            continue;
        if (emitted)
            out << ",";
        const double density = instruction_count > 0
            ? static_cast<double>(reg.touches) /
              static_cast<double>(instruction_count)
            : 0.0;
        out << reg.reg << ":" << std::fixed << std::setprecision(2) << density;
        if (++emitted == 3)
            break;
    }
    return out.str();
}

void print_reg_scope_header(size_t name_width) {
    std::cout << std::left << std::setw((int)name_width) << "scope"
              << std::right
              << std::setw(8) << "line"
              << std::setw(8) << "insn"
              << std::setw(10) << "press"
              << std::setw(8) << "br"
              << std::setw(8) << "call"
              << std::setw(8) << "mem"
              << std::setw(8) << "alt"
              << std::setw(8) << "swap"
              << "  hot"
              << std::setw(28) << "cold"
              << "\n";
    std::cout << std::string(name_width + 102, '-') << "\n";
}

void print_reg_scope_row(size_t name_width,
                         const xopt::register_coverage_scope &scope) {
    std::cout << std::left << std::setw((int)name_width) << scope.name
              << std::right
              << std::setw(8) << scope.first_line
              << std::setw(8) << scope.instruction_count
              << std::setw(10) << std::fixed << std::setprecision(2)
              << scope.pressure
              << std::setw(8) << scope.branch_count
              << std::setw(8) << scope.call_count
              << std::setw(8) << scope.memory_touch_count
              << std::setw(8) << scope.alternate_touch_count
              << std::setw(8) << scope.alternate_switch_count
              << "  " << std::left << std::setw(24)
              << reg_density_list(scope.registers, scope.instruction_count, true)
              << reg_density_list(scope.registers, scope.instruction_count, false)
              << "\n";
}

void print_reg_windows(const xopt::register_coverage_report &report) {
    if (report.hot_windows.empty())
        return;

    std::cout << "\nhot windows:\n";
    std::cout << std::left << std::setw(28) << "scope"
              << std::right
              << std::setw(8) << "line"
              << std::setw(8) << "insn"
              << std::setw(10) << "press"
              << std::setw(8) << "alt"
              << std::setw(8) << "swap"
              << "  hot"
              << std::setw(28) << "cold"
              << "\n";
    std::cout << std::string(102, '-') << "\n";
    for (const auto &window : report.hot_windows) {
        std::cout << std::left << std::setw(28) << window.scope
                  << std::right
                  << std::setw(8) << window.first_line
                  << std::setw(8) << window.instruction_count
                  << std::setw(10) << std::fixed << std::setprecision(2)
                  << window.pressure
                  << std::setw(8) << window.alternate_touch_count
                  << std::setw(8) << window.alternate_switch_count
                  << "  " << std::left << std::setw(24)
                  << reg_density_list(window.registers,
                                      window.instruction_count, true)
                  << reg_density_list(window.registers,
                                      window.instruction_count, false)
                  << "\n";
    }
}

void print_reg_coverage_report(const std::string &name,
                               xopt::register_coverage_report report) {
    std::cout << "== " << name << " ==\n";
    size_t name_width = std::max<size_t>(16, report.total.name.size() + 2);
    for (const auto &routine : report.routines)
        name_width = std::max(name_width, routine.name.size() + 2);
    name_width = std::min<size_t>(name_width, 44);

    print_reg_scope_header(name_width);
    print_reg_scope_row(name_width, report.total);

    std::sort(report.routines.begin(), report.routines.end(),
              [](const auto &a, const auto &b) {
                  if (a.pressure != b.pressure)
                      return a.pressure > b.pressure;
                  return a.instruction_count > b.instruction_count;
              });
    const size_t limit = std::min<size_t>(report.routines.size(), 12);
    for (size_t i = 0; i < limit; ++i)
        print_reg_scope_row(name_width, report.routines[i]);

    print_reg_windows(report);
}

int run_reg_coverage(const cli_options &opts) {
    if (opts.optimizer.cross_file) {
        print_reg_coverage_report(
            "<cross-file>",
            xopt::analyze_z80_register_coverage(combine_inputs(opts.inputs)));
        return 0;
    }

    for (const auto &input : opts.inputs) {
        print_reg_coverage_report(
            input,
            xopt::analyze_z80_register_coverage(read_text_file(input)));
    }
    return 0;
}

int run(const cli_options &opts) {
    if (opts.stats)
        return run_stats(opts);
    if (opts.reg_coverage)
        return run_reg_coverage(opts);

    if (opts.optimizer.cross_file) {
        std::string optimized =
            xopt::optimize_assembly(combine_inputs(opts.inputs), opts.optimizer);
        if (!opts.output.empty()) {
            write_text_file(opts.output, optimized);
        } else {
            std::cout << optimized;
        }
        return 0;
    }

    if (!opts.out_dir.empty())
        std::filesystem::create_directories(opts.out_dir);

    for (const auto &input : opts.inputs) {
        std::string optimized =
            xopt::optimize_assembly(read_text_file(input), opts.optimizer);
        if (opts.in_place) {
            write_text_file(input, optimized);
        } else if (!opts.out_dir.empty()) {
            write_text_file(out_dir_path(opts.out_dir, input), optimized);
        } else if (!opts.output.empty()) {
            write_text_file(opts.output, optimized);
        } else {
            std::cout << optimized;
        }
    }
    return 0;
}

} // namespace

int main(int argc, char **argv) {
    try {
        return run(parse_args(argc, argv));
    } catch (const std::exception &e) {
        std::cerr << "xopt: error: " << e.what() << "\n";
        return 1;
    }
}
