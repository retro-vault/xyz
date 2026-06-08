//
// main.cpp
//
// xobjcopy — Z80 object/archive copy and conversion tool for the xyz toolchain.
// Uses libxbfd for all supported format reading and writing.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <iostream>

#include <xobjcopy/cli.h>
#include <xobjcopy/errors.h>
#include <xobjcopy/operations.h>

#define XOBJCOPY_VERSION "1.0.0"

int main(int argc, char* argv[])
{
    try {
        auto opts = xobjcopy::cli::parse(argc, argv);

        if (opts.show_help) {
            xobjcopy::cli::print_usage(argc > 0 ? argv[0] : "xobjcopy");
            return 0;
        }

        if (opts.show_version) {
            std::cout << "xobjcopy " << XOBJCOPY_VERSION
                      << " (X Object Copy for Z80)\n";
            return 0;
        }

        xobjcopy::run(opts);
        return 0;
    } catch (const xobjcopy::usage_error& e) {
        std::cerr << "xobjcopy: error: " << e.what() << "\n";
        return 1;
    } catch (const xobjcopy::error& e) {
        std::cerr << "xobjcopy: error: " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "xobjcopy: internal error: " << e.what() << "\n";
        return 2;
    }
}
