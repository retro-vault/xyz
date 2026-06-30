#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <cerrno>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

#include <xbfd/xbfd.h>
#include <xemu/xemu.h>

namespace fs = std::filesystem;

namespace {

constexpr uint16_t k_emu_result_addr = 0xff00;
constexpr uint16_t k_emu_done_addr = 0xff02;
constexpr uint8_t k_emu_done_magic = 0xa5;

struct cli_options {
    fs::path xcc;
    std::string gcc = "gcc";
    fs::path suite_root;
    fs::path work_root;
    std::optional<std::string> filter;
    bool list_only = false;
    bool verbose = false;
};

enum class test_kind {
    compile,
    run
};

enum class runner_kind {
    xemu,
    command
};

enum class compile_expectation {
    success,
    failure
};

enum class host_golden_kind {
    none,
    gcc
};

enum class value_kind {
    s8,
    u8,
    s16,
    u16,
    s32,
    u32,
    s64,
    u64
};

struct register_expectation {
    std::string name;
    uint16_t value = 0;
};

struct memory_expectation {
    uint16_t address = 0;
    std::vector<uint8_t> bytes;
};

struct variable_expectation {
    std::string name;
    value_kind kind = value_kind::s16;
    std::int64_t value = 0;
};

struct test_case {
    fs::path manifest_path;
    fs::path directory;
    std::string id;
    runner_kind runner = runner_kind::xemu;
    test_kind kind = test_kind::run;
    compile_expectation expect_compile = compile_expectation::success;
    host_golden_kind host_golden = host_golden_kind::none;
    std::string component;
    std::string summary;
    std::vector<std::string> tags;
    std::vector<std::string> aliases;
    std::vector<std::string> legacy_paths;
    std::vector<fs::path> sources;
    std::vector<std::string> compiler_args;
    std::vector<std::string> host_args;
    std::vector<std::string> stderr_contains;
    std::vector<std::string> stderr_not_contains;
    std::vector<std::string> asm_contains;
    std::vector<std::string> asm_not_contains;
    std::vector<std::string> matrix_opts;
    std::vector<std::string> matrix_floats;
    std::optional<fs::path> stdin_path;
    std::optional<fs::path> stdout_path;
    std::optional<int> expect_exit;
    std::size_t max_steps = 1'000'000;
    int timeout_seconds = 20;
    uint16_t origin = 0x0000;
    std::optional<uint16_t> pc;
    uint16_t sp = 0xffff;
    std::string platform = "emu";
    std::optional<uint16_t> stdin_status_port = 0x00e2;
    uint16_t stdin_data_port = 0x00e3;
    uint16_t stdout_port = 0x00e1;
    fs::path command_workdir = ".";
    std::vector<std::string> command_args;
    bool float_present = false;
    bool debug_symbols = false;
    std::vector<register_expectation> register_assertions;
    std::vector<memory_expectation> memory_assertions;
    std::vector<variable_expectation> variable_assertions;
};

struct expanded_test_case {
    test_case base;
    std::string variant_id;
    std::optional<std::string> opt_level;
    std::optional<std::string> float_format;
};

struct command_result {
    int exit_code = -1;
    bool timed_out = false;
    std::string stdout_text;
    std::string stderr_text;
};

struct golden_result {
    int exit_code = -1;
    std::string stdout_text;
};

struct test_result {
    bool passed = false;
    std::string detail;
};

[[noreturn]] void fail(const std::string& message) {
    throw std::runtime_error(message);
}

std::string trim(std::string_view text) {
    std::size_t start = 0;
    while (start < text.size() &&
           std::isspace(static_cast<unsigned char>(text[start])) != 0) {
        ++start;
    }

    std::size_t end = text.size();
    while (end > start &&
           std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
        --end;
    }

    return std::string(text.substr(start, end - start));
}

std::string to_lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return text;
}

std::string normalize_key(const std::string& value) {
    std::string key = to_lower(trim(value));
    key.erase(std::remove_if(key.begin(), key.end(), [](unsigned char ch) {
        return ch == '_' || ch == '-';
    }), key.end());
    return key;
}

std::string sanitize_id_component(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        if (std::isalnum(ch) != 0) {
            return static_cast<char>(ch);
        }
        return '_';
    });
    value.erase(
        std::unique(value.begin(), value.end(), [](char lhs, char rhs) {
            return lhs == '_' && rhs == '_';
        }),
        value.end());
    while (!value.empty() && value.front() == '_') {
        value.erase(value.begin());
    }
    while (!value.empty() && value.back() == '_') {
        value.pop_back();
    }
    return value;
}

std::uint32_t parse_u32(const std::string& value) {
    char* end = nullptr;
    errno = 0;
    const unsigned long parsed = std::strtoul(value.c_str(), &end, 0);
    if (errno != 0 || end == value.c_str() || *end != '\0') {
        fail("invalid number: " + value);
    }
    return static_cast<std::uint32_t>(parsed);
}

int parse_int(const std::string& value) {
    char* end = nullptr;
    errno = 0;
    const long parsed = std::strtol(value.c_str(), &end, 0);
    if (errno != 0 || end == value.c_str() || *end != '\0') {
        fail("invalid integer: " + value);
    }
    return static_cast<int>(parsed);
}

std::int64_t parse_i64(const std::string& value) {
    char* end = nullptr;
    errno = 0;
    const long long parsed = std::strtoll(value.c_str(), &end, 0);
    if (errno != 0 || end == value.c_str() || *end != '\0') {
        fail("invalid signed integer: " + value);
    }
    return static_cast<std::int64_t>(parsed);
}

bool parse_bool(const std::string& value, const std::string& field_name) {
    const auto key = normalize_key(value);
    if (key == "1" || key == "true" || key == "yes" || key == "on") {
        return true;
    }
    if (key == "0" || key == "false" || key == "no" || key == "off") {
        return false;
    }
    fail("invalid boolean for " + field_name + ": " + value);
}

std::vector<uint8_t> read_file_bytes(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open()) {
        fail("cannot open file: " + path.string());
    }

    return std::vector<uint8_t>(
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>());
}

std::string read_file_text(const fs::path& path) {
    const auto bytes = read_file_bytes(path);
    return std::string(bytes.begin(), bytes.end());
}

std::string first_lines(const std::string& text, std::size_t max_lines = 12) {
    std::istringstream input(text);
    std::ostringstream output;
    std::string line;
    std::size_t count = 0;
    while (count < max_lines && std::getline(input, line)) {
        output << line << '\n';
        ++count;
    }
    return output.str();
}

std::string describe_bytes(const std::string& text) {
    std::ostringstream output;
    output << "size=" << text.size() << " text=\"";
    for (const unsigned char ch : text) {
        switch (ch) {
        case '\n':
            output << "\\n";
            break;
        case '\r':
            output << "\\r";
            break;
        case '\t':
            output << "\\t";
            break;
        case '\\':
            output << "\\\\";
            break;
        case '"':
            output << "\\\"";
            break;
        default:
            if (std::isprint(ch) != 0) {
                output << static_cast<char>(ch);
            } else {
                output << "\\x"
                       << std::hex << std::setw(2) << std::setfill('0')
                       << static_cast<int>(ch)
                       << std::dec << std::setfill(' ');
            }
            break;
        }
    }
    output << "\" hex=";
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (i != 0) {
            output << ' ';
        }
        output << std::hex << std::setw(2) << std::setfill('0')
               << static_cast<int>(static_cast<unsigned char>(text[i]))
               << std::dec << std::setfill(' ');
    }
    return output.str();
}

runner_kind parse_runner_kind(const std::string& value, const fs::path& manifest_path) {
    const auto kind = normalize_key(value);
    if (kind == "xemu") {
        return runner_kind::xemu;
    }
    if (kind == "command") {
        return runner_kind::command;
    }
    fail("unknown runner in " + manifest_path.string() + ": " + value);
}

host_golden_kind parse_host_golden_kind(
    const std::string& value,
    const fs::path& manifest_path)
{
    const auto kind = normalize_key(value);
    if (kind == "none") {
        return host_golden_kind::none;
    }
    if (kind == "gcc") {
        return host_golden_kind::gcc;
    }
    fail("unknown host_golden in " + manifest_path.string() + ": " + value);
}

std::string normalize_float_format(const std::string& value) {
    const auto kind = normalize_key(value);
    if (kind == "ieee" || kind == "ieee32") {
        return "ieee32";
    }
    if (kind == "ieee16" || kind == "half" || kind == "binary16") {
        return "ieee16";
    }
    if (kind == "88" || kind == "fixed88" || kind == "fixed8_8") {
        return "fixed8_8";
    }
    if (kind == "1616" || kind == "fixed1616" || kind == "fixed16_16") {
        return "fixed16_16";
    }
    if (kind == "248" || kind == "168" || kind == "fixed248"
        || kind == "fixed168") {
        return "fixed16_8";
    }
    fail("unknown float format: " + value);
}

std::string compiler_float_format(const std::string& value) {
    const auto normalized = normalize_float_format(value);
    if (normalized == "fixed16_8") {
        return "fixed24_8";
    }
    return normalized;
}

register_expectation parse_register_expectation(const std::string& value) {
    const auto eq = value.find('=');
    if (eq == std::string::npos) {
        fail("register assertion must look like <reg>=<value>");
    }

    register_expectation result;
    result.name = normalize_key(value.substr(0, eq));
    result.value = static_cast<uint16_t>(parse_u32(trim(value.substr(eq + 1))));
    return result;
}

memory_expectation parse_memory_expectation(const std::string& value) {
    const auto colon = value.find(':');
    if (colon == std::string::npos) {
        fail("memory assertion must look like <addr>: <bytes>");
    }

    memory_expectation result;
    result.address = static_cast<uint16_t>(parse_u32(trim(value.substr(0, colon))));

    std::string bytes_text = value.substr(colon + 1);
    std::replace(bytes_text.begin(), bytes_text.end(), ',', ' ');
    std::istringstream input(bytes_text);
    std::string token;
    while (input >> token) {
        const auto parsed = parse_u32(token);
        if (parsed > 0xffu) {
            fail("memory byte out of range in assertion: " + token);
        }
        result.bytes.push_back(static_cast<uint8_t>(parsed));
    }

    if (result.bytes.empty()) {
        fail("memory assertion must include at least one byte");
    }

    return result;
}

value_kind parse_value_kind(const std::string& raw) {
    const auto kind = normalize_key(raw);
    if (kind == "char" || kind == "s8" || kind == "int8") return value_kind::s8;
    if (kind == "uchar" || kind == "u8" || kind == "uint8" || kind == "byte")
        return value_kind::u8;
    if (kind == "short" || kind == "int" || kind == "s16" || kind == "int16")
        return value_kind::s16;
    if (kind == "ushort" || kind == "uint" || kind == "u16" || kind == "uint16")
        return value_kind::u16;
    if (kind == "long" || kind == "s32" || kind == "int32") return value_kind::s32;
    if (kind == "ulong" || kind == "u32" || kind == "uint32") return value_kind::u32;
    if (kind == "longlong" || kind == "s64" || kind == "int64") return value_kind::s64;
    if (kind == "ulonglong" || kind == "u64" || kind == "uint64")
        return value_kind::u64;
    fail("unsupported variable assertion type: " + raw);
}

variable_expectation parse_variable_expectation(const std::string& value) {
    const auto eq = value.find('=');
    if (eq == std::string::npos) {
        fail("variable assertion must look like <name>[:type]=<value>");
    }

    const std::string lhs = trim(value.substr(0, eq));
    const std::string rhs = trim(value.substr(eq + 1));
    const auto colon = lhs.find(':');

    variable_expectation expect;
    expect.name = trim(lhs.substr(0, colon));
    if (expect.name.empty()) {
        fail("variable assertion name cannot be empty");
    }
    if (colon != std::string::npos) {
        expect.kind = parse_value_kind(trim(lhs.substr(colon + 1)));
    }
    expect.value = parse_i64(rhs);
    return expect;
}

std::string replace_all(std::string text, const std::string& from, const std::string& to) {
    if (from.empty()) {
        return text;
    }

    std::size_t pos = 0;
    while ((pos = text.find(from, pos)) != std::string::npos) {
        text.replace(pos, from.size(), to);
        pos += to.size();
    }
    return text;
}

fs::path x_root_from_suite_root(const fs::path& suite_root) {
    return suite_root.parent_path().parent_path();
}

fs::path repo_root_from_suite_root(const fs::path& suite_root) {
    return x_root_from_suite_root(suite_root).parent_path();
}

fs::path test_work_dir(const cli_options& cli, const expanded_test_case& test) {
    return cli.work_root / test.variant_id;
}

std::string expand_placeholders(
    std::string text,
    const cli_options& cli,
    const expanded_test_case& test)
{
    const fs::path suite_root = fs::absolute(cli.suite_root);
    const fs::path x_root = fs::absolute(x_root_from_suite_root(suite_root));
    const fs::path repo_root = fs::absolute(repo_root_from_suite_root(suite_root));
    const fs::path suite_dir = fs::absolute(test.base.directory);
    const fs::path work_dir = fs::absolute(test_work_dir(cli, test));
    const fs::path source_path = test.base.sources.empty()
        ? fs::path()
        : fs::absolute(test.base.sources.front());
    const fs::path source_dir = source_path.empty()
        ? fs::path()
        : source_path.parent_path();

    text = replace_all(text, "{id}", test.base.id);
    text = replace_all(text, "{variant_id}", test.variant_id);
    text = replace_all(text, "{opt_level}", test.opt_level.value_or(""));
    text = replace_all(text, "{float_format}", test.float_format.value_or(""));
    text = replace_all(text, "{xcc}", fs::absolute(cli.xcc).string());
    text = replace_all(text, "{gcc}", cli.gcc);
    text = replace_all(text, "{suite_root}", suite_root.string());
    text = replace_all(text, "{suite_dir}", suite_dir.string());
    text = replace_all(text, "{work_dir}", work_dir.string());
    text = replace_all(text, "{x_root}", x_root.string());
    text = replace_all(text, "{repo_root}", repo_root.string());
    text = replace_all(text, "{source_path}", source_path.string());
    text = replace_all(text, "{source_dir}", source_dir.string());
    return text;
}

fs::path expand_path_placeholders(
    const fs::path& value,
    const cli_options& cli,
    const expanded_test_case& test)
{
    fs::path expanded = fs::path(expand_placeholders(value.string(), cli, test));
    if (expanded.is_relative()) {
        expanded = fs::absolute(fs::absolute(test.base.directory) / expanded);
    }
    return expanded;
}

command_result run_command(
    const std::vector<std::string>& args,
    const fs::path& workdir,
    const fs::path& stdout_log,
    const fs::path& stderr_log,
    const std::optional<fs::path>& stdin_path,
    std::optional<int> timeout_seconds,
    bool verbose)
{
    if (args.empty()) {
        fail("cannot execute an empty command");
    }

    fs::create_directories(stdout_log.parent_path());

    if (verbose) {
        std::cerr << "[xemutest] exec:";
        for (const auto& arg : args) {
            std::cerr << ' ' << arg;
        }
        std::cerr << " (cwd=" << workdir << ")\n";
    }

    pid_t pid = fork();
    if (pid < 0) {
        fail("fork failed");
    }

    if (pid == 0) {
        const int stdout_fd = ::open(
            stdout_log.c_str(),
            O_WRONLY | O_CREAT | O_TRUNC,
            0644);
        const int stderr_fd = ::open(
            stderr_log.c_str(),
            O_WRONLY | O_CREAT | O_TRUNC,
            0644);
        if (stdout_fd < 0 || stderr_fd < 0) {
            _exit(127);
        }

        int stdin_fd = -1;
        if (stdin_path.has_value()) {
            stdin_fd = ::open(stdin_path->c_str(), O_RDONLY);
        } else {
            stdin_fd = ::open("/dev/null", O_RDONLY);
        }
        if (stdin_fd < 0) {
            _exit(127);
        }

        if (::chdir(workdir.c_str()) != 0) {
            _exit(127);
        }
        if (::dup2(stdin_fd, STDIN_FILENO) < 0
            || ::dup2(stdout_fd, STDOUT_FILENO) < 0
            || ::dup2(stderr_fd, STDERR_FILENO) < 0) {
            _exit(127);
        }

        ::close(stdin_fd);
        ::close(stdout_fd);
        ::close(stderr_fd);

        std::vector<char*> argv;
        argv.reserve(args.size() + 1);
        for (const auto& arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        argv.push_back(nullptr);
        ::execvp(argv[0], argv.data());
        _exit(127);
    }

    command_result result;
    int status = 0;
    const auto start = std::chrono::steady_clock::now();
    bool finished = false;

    while (!finished) {
        const pid_t wait_result = ::waitpid(pid, &status, WNOHANG);
        if (wait_result == pid) {
            finished = true;
            break;
        }
        if (wait_result < 0) {
            fail("waitpid failed");
        }

        if (timeout_seconds.has_value()) {
            const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - start);
            if (elapsed.count() >= timeout_seconds.value()) {
                result.timed_out = true;
                ::kill(pid, SIGTERM);
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                ::kill(pid, SIGKILL);
                ::waitpid(pid, &status, 0);
                finished = true;
                break;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }

    if (result.timed_out) {
        result.exit_code = -1;
    } else if (WIFEXITED(status)) {
        result.exit_code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        result.exit_code = 128 + WTERMSIG(status);
    } else {
        result.exit_code = -1;
    }

    result.stdout_text = read_file_text(stdout_log);
    result.stderr_text = read_file_text(stderr_log);
    return result;
}

test_case load_test_case(const fs::path& manifest_path) {
    std::ifstream input(manifest_path);
    if (!input.is_open()) {
        fail("cannot open manifest: " + manifest_path.string());
    }

    test_case test;
    test.manifest_path = manifest_path;
    test.directory = manifest_path.parent_path();
    bool saw_id = false;

    std::string line;
    std::size_t line_no = 0;
    while (std::getline(input, line)) {
        ++line_no;
        const auto comment = line.find('#');
        if (comment != std::string::npos) {
            line.erase(comment);
        }
        line = trim(line);
        if (line.empty()) {
            continue;
        }

        const auto eq = line.find('=');
        if (eq == std::string::npos) {
            fail(manifest_path.string() + ":" + std::to_string(line_no)
                 + ": expected key = value");
        }

        const std::string key = normalize_key(line.substr(0, eq));
        const std::string value = trim(line.substr(eq + 1));

        if (key == "id") {
            test.id = value;
            saw_id = true;
        } else if (key == "runner") {
            test.runner = parse_runner_kind(value, manifest_path);
        } else if (key == "kind") {
            const auto kind = normalize_key(value);
            if (kind == "compile") {
                test.kind = test_kind::compile;
            } else if (kind == "run") {
                test.kind = test_kind::run;
            } else {
                fail("unknown test kind in " + manifest_path.string() + ": " + value);
            }
        } else if (key == "component") {
            test.component = value;
        } else if (key == "summary") {
            test.summary = value;
        } else if (key == "tag") {
            test.tags.push_back(value);
        } else if (key == "alias") {
            test.aliases.push_back(value);
        } else if (key == "legacypath") {
            test.legacy_paths.push_back(value);
        } else if (key == "source") {
            test.sources.push_back(test.directory / value);
        } else if (key == "compilerarg") {
            test.compiler_args.push_back(value);
        } else if (key == "hostarg") {
            test.host_args.push_back(value);
        } else if (key == "command") {
            test.command_args.push_back(value);
        } else if (key == "commandarg") {
            test.command_args.push_back(value);
        } else if (key == "workdir") {
            test.command_workdir = value;
        } else if (key == "expectcompile") {
            const auto expect = normalize_key(value);
            if (expect == "success") {
                test.expect_compile = compile_expectation::success;
            } else if (expect == "failure") {
                test.expect_compile = compile_expectation::failure;
            } else {
                fail("unknown compile expectation in " + manifest_path.string()
                     + ": " + value);
            }
        } else if (key == "hostgolden") {
            test.host_golden = parse_host_golden_kind(value, manifest_path);
        } else if (key == "stderrcontains") {
            test.stderr_contains.push_back(value);
        } else if (key == "stderrnotcontains") {
            test.stderr_not_contains.push_back(value);
        } else if (key == "asmcontains") {
            test.asm_contains.push_back(value);
        } else if (key == "asmnotcontains") {
            test.asm_not_contains.push_back(value);
        } else if (key == "stdin") {
            test.stdin_path = test.directory / value;
        } else if (key == "stdout") {
            test.stdout_path = test.directory / value;
        } else if (key == "expectexit") {
            test.expect_exit = parse_int(value);
        } else if (key == "maxsteps") {
            test.max_steps = static_cast<std::size_t>(parse_u32(value));
        } else if (key == "timeoutseconds") {
            test.timeout_seconds = parse_int(value);
        } else if (key == "origin") {
            test.origin = static_cast<uint16_t>(parse_u32(value));
        } else if (key == "pc") {
            test.pc = static_cast<uint16_t>(parse_u32(value));
        } else if (key == "sp") {
            test.sp = static_cast<uint16_t>(parse_u32(value));
        } else if (key == "platform") {
            test.platform = value;
        } else if (key == "floatpresent") {
            test.float_present = parse_bool(value, "float_present");
        } else if (key == "debugsymbols") {
            test.debug_symbols = parse_bool(value, "debug_symbols");
        } else if (key == "matrixopt") {
            test.matrix_opts.push_back(value);
        } else if (key == "matrixfloat") {
            test.matrix_floats.push_back(normalize_float_format(value));
        } else if (key == "stdinstatusport") {
            if (normalize_key(value) == "none") {
                test.stdin_status_port.reset();
            } else {
                test.stdin_status_port = static_cast<uint16_t>(parse_u32(value));
            }
        } else if (key == "stdindataport") {
            test.stdin_data_port = static_cast<uint16_t>(parse_u32(value));
        } else if (key == "stdoutport") {
            test.stdout_port = static_cast<uint16_t>(parse_u32(value));
        } else if (key == "assertreg") {
            test.register_assertions.push_back(parse_register_expectation(value));
        } else if (key == "assertmem") {
            test.memory_assertions.push_back(parse_memory_expectation(value));
        } else if (key == "assertvar") {
            test.variable_assertions.push_back(parse_variable_expectation(value));
        } else {
            fail("unknown manifest key in " + manifest_path.string() + ": " + key);
        }
    }

    if (!saw_id || test.id.empty()) {
        fail("test is missing required id: " + manifest_path.string());
    }

    if (test.runner == runner_kind::command) {
        if (test.command_args.empty()) {
            fail("command runner requires command or command_arg entries: "
                 + manifest_path.string());
        }
    } else if (test.sources.empty()) {
        fail("test has no sources: " + manifest_path.string());
    }

    return test;
}

void register_alias(
    std::set<std::string>& seen,
    const std::string& name,
    const std::string& owner)
{
    if (name.empty()) {
        return;
    }
    if (!seen.insert(name).second) {
        fail("duplicate manifest alias/path/id: " + name + " (seen at " + owner + ")");
    }
}

std::vector<test_case> discover_tests(const fs::path& suite_root) {
    std::vector<fs::path> manifests;
    if (!fs::exists(suite_root) || !fs::is_directory(suite_root)) {
        fail("suite root does not exist: " + suite_root.string());
    }

    for (const auto& entry : fs::recursive_directory_iterator(suite_root)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        if (entry.path().filename() == "test.cfg") {
            manifests.push_back(entry.path());
        }
    }

    std::sort(manifests.begin(), manifests.end());

    std::vector<test_case> tests;
    tests.reserve(manifests.size());
    std::set<std::string> seen_names;
    for (const auto& manifest : manifests) {
        tests.push_back(load_test_case(manifest));
        const auto& test = tests.back();
        const std::string owner = test.manifest_path.string();
        register_alias(seen_names, test.id, owner);
        for (const auto& alias : test.aliases) {
            register_alias(seen_names, alias, owner);
        }
        for (const auto& legacy_path : test.legacy_paths) {
            register_alias(seen_names, legacy_path, owner);
        }
    }
    return tests;
}

std::vector<expanded_test_case> expand_test_cases(const std::vector<test_case>& tests) {
    std::vector<expanded_test_case> expanded;
    for (const auto& test : tests) {
        std::vector<std::optional<std::string>> opts;
        opts.push_back(std::nullopt);
        if (!test.matrix_opts.empty()) {
            opts.clear();
            for (const auto& opt : test.matrix_opts) {
                opts.push_back(opt);
            }
        }

        std::vector<std::optional<std::string>> floats;
        floats.push_back(std::nullopt);
        if (!test.matrix_floats.empty()) {
            floats.clear();
            for (const auto& fmt : test.matrix_floats) {
                floats.push_back(fmt);
            }
        } else if (test.float_present) {
            floats = {
                std::optional<std::string>("ieee32"),
                std::optional<std::string>("ieee16"),
                std::optional<std::string>("fixed8_8"),
                std::optional<std::string>("fixed16_16"),
                std::optional<std::string>("fixed16_8")
            };
        }

        for (const auto& opt : opts) {
            for (const auto& fmt : floats) {
                expanded_test_case item;
                item.base = test;
                item.opt_level = opt;
                item.float_format = fmt;
                item.variant_id = test.id;
                if (opt.has_value()) {
                    item.variant_id += "__" + sanitize_id_component(opt.value());
                }
                if (fmt.has_value()) {
                    item.variant_id += "__" + sanitize_id_component(fmt.value());
                }
                expanded.push_back(std::move(item));
            }
        }
    }
    return expanded;
}

bool text_contains_all(
    const std::string& text,
    const std::vector<std::string>& patterns,
    std::string* missing)
{
    for (const auto& pattern : patterns) {
        if (text.find(pattern) == std::string::npos) {
            if (missing != nullptr) {
                *missing = pattern;
            }
            return false;
        }
    }
    return true;
}

bool text_contains_any(
    const std::string& text,
    const std::vector<std::string>& patterns,
    std::string* found)
{
    for (const auto& pattern : patterns) {
        if (text.find(pattern) != std::string::npos) {
            if (found != nullptr) {
                *found = pattern;
            }
            return true;
        }
    }
    return false;
}

bool test_matches_filter(const expanded_test_case& test, const std::string& filter) {
    if (test.variant_id.find(filter) != std::string::npos) {
        return true;
    }
    if (test.base.id.find(filter) != std::string::npos) {
        return true;
    }
    if (test.base.component.find(filter) != std::string::npos) {
        return true;
    }
    for (const auto& alias : test.base.aliases) {
        if (alias.find(filter) != std::string::npos) {
            return true;
        }
    }
    for (const auto& tag : test.base.tags) {
        if (tag.find(filter) != std::string::npos) {
            return true;
        }
    }
    for (const auto& legacy_path : test.base.legacy_paths) {
        if (legacy_path.find(filter) != std::string::npos) {
            return true;
        }
    }
    return false;
}

uint16_t read_register_by_name(
    const xemu::register_image& regs,
    const std::string& raw_name)
{
    const auto name = normalize_key(raw_name);
    if (name == "af") return regs.af;
    if (name == "bc") return regs.bc;
    if (name == "de") return regs.de;
    if (name == "hl") return regs.hl;
    if (name == "ix") return regs.ix;
    if (name == "iy") return regs.iy;
    if (name == "sp") return regs.sp;
    if (name == "pc") return regs.pc;
    if (name == "i") return regs.i;
    if (name == "r") return regs.r;
    if (name == "a") return static_cast<uint16_t>((regs.af >> 8) & 0xff);
    if (name == "f") return static_cast<uint16_t>(regs.af & 0xff);
    if (name == "b") return static_cast<uint16_t>((regs.bc >> 8) & 0xff);
    if (name == "c") return static_cast<uint16_t>(regs.bc & 0xff);
    if (name == "d") return static_cast<uint16_t>((regs.de >> 8) & 0xff);
    if (name == "e") return static_cast<uint16_t>(regs.de & 0xff);
    if (name == "h") return static_cast<uint16_t>((regs.hl >> 8) & 0xff);
    if (name == "l") return static_cast<uint16_t>(regs.hl & 0xff);
    fail("unknown register name: " + raw_name);
}

int decode_emu_exit_code(xemu::machine& emu, bool* done_seen) {
    const uint8_t done = emu.read_byte(k_emu_done_addr);
    if (done_seen != nullptr) {
        *done_seen = (done == k_emu_done_magic);
    }

    const uint16_t raw =
        static_cast<uint16_t>(emu.read_byte(k_emu_result_addr))
        | static_cast<uint16_t>(
            static_cast<uint16_t>(emu.read_byte(k_emu_result_addr + 1)) << 8);
    return static_cast<int16_t>(raw);
}

std::string stop_reason_name(xemu::stop_reason reason) {
    switch (reason) {
    case xemu::stop_reason::breakpoint:
        return "breakpoint";
    case xemu::stop_reason::step_limit:
        return "step-limit";
    case xemu::stop_reason::stepped:
        return "stepped";
    case xemu::stop_reason::none:
        return "none";
    case xemu::stop_reason::halted:
        return "halted";
    case xemu::stop_reason::fault:
        return "fault";
    }
    return "unknown";
}

std::vector<std::string> build_compiler_args(
    const expanded_test_case& test,
    const cli_options& cli)
{
    std::vector<std::string> args;
    for (const auto& arg : test.base.compiler_args) {
        args.push_back(expand_placeholders(arg, cli, test));
    }
    if (test.opt_level.has_value()) {
        args.push_back("-" + test.opt_level.value());
    }
    if (test.float_format.has_value()) {
        args.push_back("--float-format="
            + compiler_float_format(test.float_format.value()));
    }
    return args;
}

std::vector<std::string> build_target_compile_command(
    const expanded_test_case& test,
    const cli_options& cli,
    const fs::path& output_path)
{
    auto shortest_cwd_path = [](const fs::path& path) -> std::string {
        const fs::path absolute = fs::absolute(path).lexically_normal();
        std::string best = absolute.string();

        std::error_code ec;
        fs::path relative = fs::relative(absolute, fs::current_path(), ec);
        if (!ec) {
            relative = relative.lexically_normal();
            const std::string rel_text = relative.string();
            if (!rel_text.empty() && rel_text.size() < best.size()) {
                best = rel_text;
            }
        }
        return best;
    };

    std::vector<std::string> args;
    args.push_back(cli.xcc.string());
    if (test.base.kind == test_kind::compile) {
        args.push_back("-S");
    } else {
        args.push_back("--platform=" + test.base.platform);
        args.push_back("--oformat=binary");
    }
    if (test.base.debug_symbols || !test.base.variable_assertions.empty()) {
        args.push_back("-g");
    }

    const auto compiler_args = build_compiler_args(test, cli);
    args.insert(args.end(), compiler_args.begin(), compiler_args.end());
    for (const auto& source : test.base.sources) {
        args.push_back(shortest_cwd_path(source));
    }
    args.push_back("-o");
    args.push_back(output_path.string());
    return args;
}

bool is_host_passthrough_arg(const std::string& arg) {
    if (arg.empty()) {
        return false;
    }
    if (arg == "-S") {
        return false;
    }
    if (arg.rfind("--", 0) == 0) {
        return false;
    }
    if (arg.rfind("-O", 0) == 0) {
        return false;
    }
    if (arg.rfind("-I", 0) == 0 || arg.rfind("-D", 0) == 0 || arg.rfind("-U", 0) == 0) {
        return true;
    }
    if (arg.rfind("-W", 0) == 0 || arg.rfind("-f", 0) == 0 || arg.rfind("-m", 0) == 0) {
        return true;
    }
    if (arg.rfind("-std=", 0) == 0) {
        return true;
    }
    return false;
}

bool is_host_link_only_arg(const std::string& arg) {
    if (arg.empty()) {
        return false;
    }
    if (arg.rfind("-l", 0) == 0 || arg.rfind("-L", 0) == 0) {
        return true;
    }
    if (arg.rfind("-Wl,", 0) == 0) {
        return true;
    }

    const fs::path path(arg);
    const std::string ext = to_lower(path.extension().string());
    return ext == ".a" || ext == ".so";
}

bool is_host_source_file(const fs::path& path) {
    const auto ext = to_lower(path.extension().string());
    return ext == ".c" || ext == ".cc" || ext == ".cpp" || ext == ".cxx";
}

std::vector<std::string> build_host_compile_command(
    const expanded_test_case& test,
    const cli_options& cli,
    const fs::path& output_path)
{
    std::vector<std::string> args;
    args.push_back(cli.gcc);

    bool saw_std = false;
    bool saw_opt = false;
    std::vector<std::string> post_source_args;
    for (const auto& arg : test.base.host_args) {
        const auto expanded = expand_placeholders(arg, cli, test);
        if (expanded.rfind("-std=", 0) == 0) {
            saw_std = true;
        }
        if (expanded.rfind("-O", 0) == 0) {
            saw_opt = true;
        }
        if (is_host_link_only_arg(expanded)) {
            post_source_args.push_back(expanded);
        } else {
            args.push_back(expanded);
        }
    }
    if (!saw_std) {
        args.push_back("-std=c2x");
    }
    if (!saw_opt) {
        args.push_back("-O0");
    }
    args.push_back("-Wno-attributes");

    for (const auto& arg : build_compiler_args(test, cli)) {
        if (is_host_passthrough_arg(arg)) {
            args.push_back(arg);
        }
    }

    for (const auto& source : test.base.sources) {
        if (!is_host_source_file(source)) {
            fail("host_golden requires host-compilable source files only: "
                 + source.string());
        }
        args.push_back(fs::absolute(source).string());
    }
    args.insert(args.end(), post_source_args.begin(), post_source_args.end());
    args.push_back("-o");
    args.push_back(output_path.string());
    return args;
}

std::optional<golden_result> prepare_host_golden(
    const expanded_test_case& test,
    const cli_options& cli)
{
    if (test.base.host_golden == host_golden_kind::none) {
        return std::nullopt;
    }

    const fs::path test_work = test_work_dir(cli, test);
    const fs::path host_bin = test_work / "host-program";
    const fs::path host_build_stdout = test_work / "host-build.stdout.log";
    const fs::path host_build_stderr = test_work / "host-build.stderr.log";
    const fs::path host_run_stdout = test_work / "host-run.stdout.log";
    const fs::path host_run_stderr = test_work / "host-run.stderr.log";
    const fs::path host_run_dir = test_work / "host-fs";

    const auto build_args = build_host_compile_command(test, cli, host_bin);
    const auto build_result = run_command(
        build_args,
        fs::current_path(),
        host_build_stdout,
        host_build_stderr,
        std::nullopt,
        test.base.timeout_seconds,
        cli.verbose);
    if (build_result.timed_out) {
        fail("host golden generation timed out");
    }
    if (build_result.exit_code != 0) {
        fail("host golden compilation failed:\n" + first_lines(build_result.stderr_text));
    }

    fs::remove_all(host_run_dir);
    fs::create_directories(host_run_dir);
    const auto run_result = run_command(
        {fs::absolute(host_bin).string()},
        host_run_dir,
        host_run_stdout,
        host_run_stderr,
        test.base.stdin_path,
        test.base.timeout_seconds,
        cli.verbose);
    if (run_result.timed_out) {
        fail("host golden execution timed out");
    }

    return golden_result{run_result.exit_code, run_result.stdout_text};
}

const xbfd::debug_function* find_function_for_pc(
    const xbfd::debug_info& info,
    std::uint32_t pc)
{
    for (const auto& function : info.functions) {
        if (pc >= function.start && pc < function.end) {
            return &function;
        }
    }
    return nullptr;
}

std::vector<const xbfd::debug_variable*> visible_variables(
    const xbfd::debug_info& info,
    std::uint32_t pc)
{
    std::vector<const xbfd::debug_variable*> result;
    const auto* function = find_function_for_pc(info, pc);
    for (const auto& variable : info.variables) {
        if (function != nullptr && !variable.parent.empty()
            && variable.parent != function->name) {
            continue;
        }
        result.push_back(&variable);
    }
    return result;
}

std::uint64_t read_little_endian(
    const xemu::machine& emu,
    std::uint16_t address,
    std::size_t width)
{
    std::uint64_t value = 0;
    for (std::size_t i = 0; i < width; ++i) {
        value |= static_cast<std::uint64_t>(
            emu.read_byte(static_cast<uint16_t>(address + i))) << (i * 8u);
    }
    return value;
}

std::int64_t sign_extend(std::uint64_t raw, std::size_t width) {
    const std::size_t bits = width * 8u;
    if (bits == 0 || bits >= 64u) {
        return static_cast<std::int64_t>(raw);
    }
    const std::uint64_t sign_bit = std::uint64_t{1} << (bits - 1u);
    if ((raw & sign_bit) == 0u) {
        return static_cast<std::int64_t>(raw);
    }
    const std::uint64_t mask = ~((std::uint64_t{1} << bits) - 1u);
    return static_cast<std::int64_t>(raw | mask);
}

std::size_t value_kind_width(value_kind kind) {
    switch (kind) {
    case value_kind::s8:
    case value_kind::u8:
        return 1;
    case value_kind::s16:
    case value_kind::u16:
        return 2;
    case value_kind::s32:
    case value_kind::u32:
        return 4;
    case value_kind::s64:
    case value_kind::u64:
        return 8;
    }
    return 0;
}

bool value_kind_signed(value_kind kind) {
    switch (kind) {
    case value_kind::s8:
    case value_kind::s16:
    case value_kind::s32:
    case value_kind::s64:
        return true;
    case value_kind::u8:
    case value_kind::u16:
    case value_kind::u32:
    case value_kind::u64:
        return false;
    }
    return false;
}

std::int64_t read_variable_value(
    const xbfd::debug_variable& variable,
    value_kind kind,
    const xemu::machine& emu,
    const xemu::register_image& regs)
{
    const std::size_t width = value_kind_width(kind);
    if (width == 0) {
        fail("unsupported variable width");
    }

    std::uint64_t raw = 0;
    switch (variable.storage) {
    case xbfd::var_storage::external:
        raw = read_little_endian(
            emu,
            static_cast<std::uint16_t>(variable.offset & 0xffff),
            width);
        break;
    case xbfd::var_storage::stack: {
        const std::uint16_t address = static_cast<std::uint16_t>(
            static_cast<std::int32_t>(regs.ix) + variable.offset);
        raw = read_little_endian(emu, address, width);
        break;
    }
    case xbfd::var_storage::reg: {
        std::string reg = normalize_key(variable.reg);
        reg.erase(std::remove(reg.begin(), reg.end(), ','), reg.end());
        raw = read_register_by_name(regs, reg);
        break;
    }
    default:
        fail("variable '" + variable.name + "' has unsupported storage");
    }

    if (value_kind_signed(kind)) {
        return sign_extend(raw, width);
    }
    return static_cast<std::int64_t>(raw);
}

test_result verify_variable_assertions(
    const expanded_test_case& test,
    const fs::path& cdb_path,
    const xemu::machine& emu,
    const xemu::register_image& regs)
{
    const auto info_opt = xbfd::debug_reader::read_cdb(cdb_path.string());
    if (!info_opt.has_value()) {
        return {false, "failed to load debug symbols: " + cdb_path.string()};
    }

    auto info = *info_opt;
    const fs::path map_path = cdb_path.parent_path()
        / (cdb_path.stem().string() + ".map");
    if (fs::exists(map_path)) {
        if (auto merged = xbfd::debug_reader::read_map(map_path.string(), info);
            merged.has_value()) {
            info = std::move(*merged);
        }
    }

    const auto vars = visible_variables(info, regs.pc);
    for (const auto& expect : test.base.variable_assertions) {
        const xbfd::debug_variable* found = nullptr;
        for (const auto* variable : vars) {
            if (variable->name == expect.name) {
                found = variable;
                break;
            }
        }
        if (found == nullptr) {
            for (const auto& variable : info.variables) {
                if (variable.name == expect.name && variable.parent.empty()) {
                    found = &variable;
                    break;
                }
            }
        }
        if (found == nullptr) {
            return {false, "debug variable not visible/found: " + expect.name};
        }

        const std::int64_t actual = read_variable_value(*found, expect.kind, emu, regs);
        if (actual != expect.value) {
            return {false, "variable mismatch for " + expect.name
                           + ": expected " + std::to_string(expect.value)
                           + ", got " + std::to_string(actual)};
        }
    }

    return {true, {}};
}

test_result run_compile_test(
    const expanded_test_case& test,
    const cli_options& cli)
{
    const fs::path test_work = test_work_dir(cli, test);
    const fs::path asm_output = test_work / "compile.s";
    const fs::path stdout_log = test_work / "compile.stdout.log";
    const fs::path stderr_log = test_work / "compile.stderr.log";

    const auto args = build_target_compile_command(test, cli, asm_output);
    const auto result = run_command(
        args,
        fs::current_path(),
        stdout_log,
        stderr_log,
        std::nullopt,
        test.base.timeout_seconds,
        cli.verbose);
    const bool compiled = result.exit_code == 0;

    if (result.timed_out) {
        return {false, "compile timed out"};
    }
    if (test.base.expect_compile == compile_expectation::success && !compiled) {
        return {false, "compile failed:\n" + first_lines(result.stderr_text)};
    }
    if (test.base.expect_compile == compile_expectation::failure && compiled) {
        return {false, "expected compile failure but compilation succeeded"};
    }

    std::string missing_pattern;
    if (!text_contains_all(result.stderr_text, test.base.stderr_contains, &missing_pattern)) {
        return {false, "expected stderr to contain: " + missing_pattern
                       + "\n" + first_lines(result.stderr_text)};
    }

    std::string found_pattern;
    if (text_contains_any(result.stderr_text, test.base.stderr_not_contains, &found_pattern)) {
        return {false, "stderr unexpectedly contained: " + found_pattern
                       + "\n" + first_lines(result.stderr_text)};
    }

    if (compiled) {
        const std::string asm_text = read_file_text(asm_output);

        if (!text_contains_all(asm_text, test.base.asm_contains, &missing_pattern)) {
            return {false, "expected assembly to contain: " + missing_pattern};
        }

        if (text_contains_any(asm_text, test.base.asm_not_contains, &found_pattern)) {
            return {false, "assembly unexpectedly contained: " + found_pattern};
        }
    }

    return {true, {}};
}

test_result run_emulated_test(
    const expanded_test_case& test,
    const cli_options& cli)
{
    const fs::path test_work = test_work_dir(cli, test);
    const fs::path image_output = test_work / "program.bin";
    const fs::path stdout_log = test_work / "link.stdout.log";
    const fs::path stderr_log = test_work / "link.stderr.log";

    const auto args = build_target_compile_command(test, cli, image_output);
    const auto compile_result = run_command(
        args,
        fs::current_path(),
        stdout_log,
        stderr_log,
        std::nullopt,
        test.base.timeout_seconds,
        cli.verbose);
    if (compile_result.timed_out) {
        return {false, "build timed out"};
    }
    if (compile_result.exit_code != 0) {
        return {false, "build failed:\n" + first_lines(compile_result.stderr_text)};
    }

    std::optional<golden_result> golden;
    try {
        golden = prepare_host_golden(test, cli);
    } catch (const std::exception& e) {
        return {false, std::string("host golden failed: ") + e.what()};
    }

    std::string stdin_bytes;
    if (test.base.stdin_path.has_value()) {
        stdin_bytes = read_file_text(*test.base.stdin_path);
    }

    std::string expected_stdout;
    if (test.base.stdout_path.has_value()) {
        expected_stdout = read_file_text(*test.base.stdout_path);
    } else if (golden.has_value()) {
        expected_stdout = golden->stdout_text;
    }

    const std::optional<int> expected_exit =
        test.base.expect_exit.has_value() ? test.base.expect_exit
                                          : std::optional<int>(golden.has_value()
                                                                   ? golden->exit_code
                                                                   : std::optional<int>{});

    std::istringstream input_stream(stdin_bytes);
    std::ostringstream output_stream;
    const fs::path emu_fs_root = test_work / "emu-fs";

    xemu::machine emu;
    emu.load_binary(image_output, test.base.origin);
    emu.set_pc(test.base.pc.value_or(test.base.origin));
    emu.set_sp(test.base.sp);
    fs::remove_all(emu_fs_root);
    emu.bind_host_filesystem(emu_fs_root);

    if (test.base.stdin_path.has_value()) {
        if (test.base.stdin_status_port.has_value()) {
            emu.bind_stdin_status_data(
                *test.base.stdin_status_port,
                test.base.stdin_data_port,
                input_stream);
        } else {
            emu.bind_stdin(test.base.stdin_data_port, input_stream);
        }
    }
    emu.bind_stdout(test.base.stdout_port, output_stream);

    const auto stop = emu.continue_execution(test.base.max_steps);
    const std::string actual_stdout = output_stream.str();
    bool done_seen = false;
    const int actual_exit = decode_emu_exit_code(emu, &done_seen);

    if (stop.reason != xemu::stop_reason::halted && !done_seen) {
        std::ostringstream detail;
        detail << "execution did not halt cleanly (reason="
               << stop_reason_name(stop.reason)
               << ", pc=0x" << std::hex << stop.pc << std::dec
               << ", steps=" << stop.steps;
        if (!stop.message.empty()) {
            detail << ", message=" << stop.message;
        }
        detail << ")";
        if (!actual_stdout.empty()) {
            detail << "\nstdout: " << describe_bytes(actual_stdout);
        }
        return {false, detail.str()};
    }

    if (test.base.stdout_path.has_value() || golden.has_value()) {
        if (actual_stdout != expected_stdout) {
            return {false, "stdout mismatch\nexpected: "
                           + describe_bytes(expected_stdout)
                           + "\nactual:   " + describe_bytes(actual_stdout)};
        }
    }

    if (expected_exit.has_value()) {
        if (!done_seen) {
            return {false, "emu exit mailbox was not completed"};
        }
        if (actual_exit != *expected_exit) {
            return {false, "exit status mismatch: expected "
                           + std::to_string(*expected_exit)
                           + ", got " + std::to_string(actual_exit)};
        }
    }

    const auto regs = emu.registers();
    for (const auto& expect : test.base.register_assertions) {
        const uint16_t actual = read_register_by_name(regs, expect.name);
        if (actual != expect.value) {
            std::ostringstream detail;
            detail << "register mismatch for " << expect.name
                   << ": expected 0x" << std::hex << expect.value
                   << ", got 0x" << actual << std::dec;
            return {false, detail.str()};
        }
    }

    for (const auto& expect : test.base.memory_assertions) {
        const auto actual = emu.read_memory(expect.address, expect.bytes.size());
        if (actual != expect.bytes) {
            std::ostringstream expected_bytes;
            std::ostringstream actual_bytes;
            for (std::size_t i = 0; i < expect.bytes.size(); ++i) {
                if (i != 0) {
                    expected_bytes << ' ';
                    actual_bytes << ' ';
                }
                expected_bytes << std::hex << std::setw(2) << std::setfill('0')
                               << static_cast<int>(expect.bytes[i]);
                actual_bytes << std::hex << std::setw(2) << std::setfill('0')
                             << static_cast<int>(actual[i]);
            }
            return {false, "memory mismatch at 0x"
                           + [&] {
                               std::ostringstream text;
                               text << std::hex << expect.address;
                               return text.str();
                           }()
                           + ": expected [" + expected_bytes.str()
                           + "], got [" + actual_bytes.str() + "]"};
        }
    }

    if (!test.base.variable_assertions.empty()) {
        const fs::path cdb_path = image_output.parent_path()
            / (image_output.stem().string() + ".cdb");
        if (!fs::exists(cdb_path)) {
            return {false, "expected debug sidecar not found: " + cdb_path.string()};
        }
        const auto vars_result = verify_variable_assertions(test, cdb_path, emu, regs);
        if (!vars_result.passed) {
            return vars_result;
        }
    }

    return {true, {}};
}

test_result run_command_test(
    const expanded_test_case& test,
    const cli_options& cli)
{
    const fs::path test_work = test_work_dir(cli, test);
    const fs::path stdout_log = test_work / "command.stdout.log";
    const fs::path stderr_log = test_work / "command.stderr.log";

    std::vector<std::string> args;
    args.reserve(test.base.command_args.size());
    for (const auto& arg : test.base.command_args) {
        args.push_back(expand_placeholders(arg, cli, test));
    }

    const fs::path workdir =
        fs::absolute(expand_path_placeholders(test.base.command_workdir, cli, test));
    const auto result = run_command(
        args,
        workdir,
        stdout_log,
        stderr_log,
        test.base.stdin_path,
        test.base.timeout_seconds,
        cli.verbose);

    if (result.timed_out) {
        return {false, "command timed out"};
    }
    if (result.exit_code != 0) {
        std::ostringstream detail;
        detail << "command failed with exit code " << result.exit_code;
        if (!result.stderr_text.empty()) {
            detail << "\nstderr:\n" << first_lines(result.stderr_text);
        } else if (!result.stdout_text.empty()) {
            detail << "\nstdout:\n" << first_lines(result.stdout_text);
        }
        return {false, detail.str()};
    }

    return {true, {}};
}

void print_help() {
    std::cout
        << "xemutest - unified X test runner backed by xemu\n"
        << "usage: xemutest --xcc PATH --suite DIR [options]\n\n"
        << "options:\n"
        << "  --xcc PATH        path to xcc\n"
        << "  --gcc PATH        path to host gcc (default: gcc)\n"
        << "  --suite DIR       root directory containing test.cfg manifests\n"
        << "  --work DIR        work/output directory (default build/tests/tools/xemutest/work)\n"
        << "  --filter TEXT     only run tests whose id, alias, tag, path, or component contains TEXT\n"
        << "  --list            list discovered tests and exit\n"
        << "  --verbose         print compile/link commands\n"
        << "  -h, --help        show this help\n";
}

cli_options parse_cli(int argc, char* argv[]) {
    cli_options options;
    options.work_root = "build/tests/tools/xemutest/work";

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            print_help();
            std::exit(0);
        } else if (arg == "--xcc") {
            if (++i >= argc) fail("--xcc requires a path");
            options.xcc = argv[i];
        } else if (arg == "--gcc") {
            if (++i >= argc) fail("--gcc requires a path");
            options.gcc = argv[i];
        } else if (arg == "--suite") {
            if (++i >= argc) fail("--suite requires a directory");
            options.suite_root = argv[i];
        } else if (arg == "--work") {
            if (++i >= argc) fail("--work requires a directory");
            options.work_root = argv[i];
        } else if (arg == "--filter") {
            if (++i >= argc) fail("--filter requires text");
            options.filter = argv[i];
        } else if (arg == "--list") {
            options.list_only = true;
        } else if (arg == "--verbose") {
            options.verbose = true;
        } else {
            fail("unknown option: " + arg);
        }
    }

    if (options.xcc.empty()) fail("--xcc is required");
    if (options.suite_root.empty()) fail("--suite is required");
    return options;
}

} // namespace

int main(int argc, char* argv[]) {
    try {
        const auto cli = parse_cli(argc, argv);
        auto tests = expand_test_cases(discover_tests(cli.suite_root));
        if (cli.filter.has_value()) {
            tests.erase(
                std::remove_if(
                    tests.begin(),
                    tests.end(),
                    [&](const expanded_test_case& test) {
                        return !test_matches_filter(test, *cli.filter);
                    }),
                tests.end());
        }

        if (tests.empty()) {
            std::cerr << "xemutest: no tests discovered\n";
            return 1;
        }

        if (cli.list_only) {
            for (const auto& test : tests) {
                std::cout << test.variant_id;
                if (!test.base.component.empty()) {
                    std::cout << " [" << test.base.component << "]";
                }
                if (!test.base.summary.empty()) {
                    std::cout << " - " << test.base.summary;
                }
                if (cli.verbose) {
                    for (const auto& alias : test.base.aliases) {
                        std::cout << " alias=" << alias;
                    }
                    for (const auto& legacy_path : test.base.legacy_paths) {
                        std::cout << " legacy=" << legacy_path;
                    }
                }
                std::cout << '\n';
            }
            return 0;
        }

        fs::create_directories(cli.work_root);

        int passed = 0;
        int failed = 0;
        for (const auto& test : tests) {
            test_result result;
            if (test.base.runner == runner_kind::command) {
                result = run_command_test(test, cli);
            } else if (test.base.kind == test_kind::compile) {
                result = run_compile_test(test, cli);
            } else {
                result = run_emulated_test(test, cli);
            }

            if (result.passed) {
                ++passed;
                std::cout << "PASS " << test.variant_id << '\n';
            } else {
                ++failed;
                std::cout << "FAIL " << test.variant_id << '\n';
                if (!result.detail.empty()) {
                    std::cout << result.detail << '\n';
                }
            }
        }

        std::cout << '\n'
                  << passed << " passed, "
                  << failed << " failed\n";
        return failed == 0 ? 0 : 1;
    } catch (const std::exception& e) {
        std::cerr << "xemutest: " << e.what() << '\n';
        return 1;
    }
}
