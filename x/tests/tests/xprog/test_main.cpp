#include <cstdint>
#include <filesystem>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <xprog/cli.h>
#include <xprog/errors.h>
#include <xprog/package.h>
#include <xprog/tape.h>

namespace {

int failures = 0;

void check(bool value, const char* expression, int line)
{
    if (!value) {
        std::cerr << "FAIL line " << line << ": " << expression << '\n';
        ++failures;
    }
}

#define CHECK(x) check((x), #x, __LINE__)

std::vector<std::uint8_t> sample_xl()
{
    return {'X', 'L', 1, 0, 2, 0, 8, 0, 1, 0, 0, 0,
            4, 0, 2, 0,
            0x00, 0x00, 0xc9, 0xcd, 0x00, 0x00, 0xc9, 0xc9};
}

xprog::cli_options parse(std::vector<std::string> args)
{
    std::vector<char*> argv;
    for (auto& arg : args)
        argv.push_back(arg.data());
    return xprog::cli::parse(static_cast<int>(argv.size()), argv.data());
}

template<typename Function>
bool throws(Function function)
{
    try { function(); } catch (const xprog::error&) { return true; }
    return false;
}

void cli_tests()
{
    auto help = parse({"xprog"});
    CHECK(help.show_help);

    auto process = parse({"xprog", "--process", "app.xl", "-o", "app.prc",
                          "--stack-size", "0x200", "--load-address", "0x8000"});
    CHECK(process.command == xprog::command_kind::process);
    CHECK(process.stack_size == 0x200);
    CHECK(process.load_address == 0x8000);
    CHECK(process.output_file == "app.prc");

    auto service = parse({"xprog", "--service", "rt.xl",
                          "--load-address", "0xfd00", "--fixed-load",
                          "--export", "2", "--export", "6"});
    CHECK(service.command == xprog::command_kind::service);
    CHECK(service.require_fixed_load);
    CHECK(service.exports.size() == 2);
    CHECK(service.output_file == "rt.svc");
    CHECK(throws([&] { parse({"xprog", "--process", "a.xl", "-o", "a.xp"}); }));
    CHECK(throws([&] {
        parse({"xprog", "--process", "--service", "a.xl",
               "--stack-size", "256"});
    }));
    CHECK(throws([&] { parse({"xprog", "a.xl"}); }));

    auto tap = parse({"xprog", "--tap", "hello.bin"});
    CHECK(tap.command == xprog::command_kind::tap);
    CHECK(tap.load_address == 0x5ccb);
    CHECK(tap.entry_point == 0x5ccb);
    CHECK(tap.output_file == "hello.tap");

    auto tzx = parse({"xprog", "--tzx", "hello.bin", "--load-address",
                      "0x8000", "--entry", "0x8001", "--name", "HELLO"});
    CHECK(tzx.command == xprog::command_kind::tzx);
    CHECK(tzx.load_address == 0x8000);
    CHECK(tzx.entry_point == 0x8001);
    CHECK(tzx.output_file == "hello.tzx");
}

void tape_tests()
{
    const std::vector<std::uint8_t> program = {0x3e, 0x2a, 0xc9};
    const auto tap = xprog::build_tap(program, 0x5ccb, 0x5ccb, "HELLO");
    std::size_t cursor = 0;
    unsigned blocks = 0;
    while (cursor < tap.size()) {
        CHECK(cursor + 2 <= tap.size());
        const auto size = static_cast<std::uint16_t>(tap[cursor])
            | static_cast<std::uint16_t>(tap[cursor + 1] << 8);
        cursor += 2;
        CHECK(size >= 2);
        CHECK(cursor + size <= tap.size());
        std::uint8_t checksum = 0;
        for (std::size_t i = 0; i < static_cast<std::size_t>(size); ++i)
            checksum ^= tap[cursor + i];
        CHECK(checksum == 0);
        cursor += size;
        ++blocks;
    }
    CHECK(blocks == 4);

    // The first data block is an auto-start BASIC program. Its line 1 REM
    // contains a machine-code loader at 0x5CD0, so loading code at 0x5CCB
    // may safely overwrite the BASIC program that launched it.
    CHECK(tap.size() > 32);
    CHECK(tap[24] == 0x00 && tap[25] == 0x01);
    CHECK(tap[28] == 0xea);
    CHECK(tap[29] == 0xdd && tap[30] == 0x21);

    const auto tzx = xprog::tap_to_tzx(tap);
    CHECK(tzx.size() > tap.size());
    CHECK(std::string(tzx.begin(), tzx.begin() + 7) == "ZXTape!");
    CHECK(tzx[7] == 0x1a);
    CHECK(tzx[8] == 1);
    CHECK(tzx[9] == 20);
    CHECK(tzx[10] == 0x10);

    CHECK(throws([&] {
        xprog::build_tap(program, 0x5ccb, 0x8000, "HELLO");
    }));
    CHECK(throws([&] {
        xprog::build_tap({}, 0x5ccb, 0x5ccb, "HELLO");
    }));
}

void package_tests()
{
    xprog::cli_options options;
    options.command = xprog::command_kind::service;
    options.name = "runtime";
    options.load_address = 0xfd00;
    options.require_fixed_load = true;
    options.exports = {2, 6};

    const auto bytes = xprog::build_image(options, sample_xl());
    const auto image = xprog::parse_image(bytes);
    CHECK(image.kind == xprog::image_kind::service);
    CHECK(image.name == "runtime");
    CHECK(image.preferred_load_address == 0xfd00);
    CHECK(image.flags & xprog::fixed_load);
    CHECK(image.exports.size() == 2);
    CHECK(image.exports[0] == 2);
    CHECK(image.xl.code_size == 8);

    const auto jump_offset = 64;
    CHECK(bytes[jump_offset] == 0xc3);
    CHECK(bytes[jump_offset + 1] == 2);

    auto damaged = bytes;
    damaged.back() ^= 1;
    CHECK(throws([&] { xprog::parse_image(damaged); }));

    options.exports = {8};
    CHECK(throws([&] { xprog::build_image(options, sample_xl()); }));

    options.exports = {2};
    auto bad_flags = xprog::build_image(options, sample_xl());
    bad_flags[7] |= 0x80;
    CHECK(throws([&] { xprog::parse_image(bad_flags); }));
}

void process_tests()
{
    xprog::cli_options options;
    options.command = xprog::command_kind::process;
    options.name = "shell";
    options.stack_size = 1024;
    const auto image = xprog::parse_image(
        xprog::build_image(options, sample_xl()));
    CHECK(image.entry_point == 2);
    CHECK(image.stack_size == 1024);
    CHECK(image.exports.empty());
    CHECK(image.image_id == xprog::name_id("shell"));

    options.stack_size.reset();
    CHECK(throws([&] { xprog::build_image(options, sample_xl()); }));

}

} // namespace

int main()
{
    cli_tests();
    package_tests();
    process_tests();
    tape_tests();
    if (failures) {
        std::cerr << failures << " xprog test(s) failed\n";
        return 1;
    }
    std::cout << "xprog tests passed\n";
    return 0;
}
