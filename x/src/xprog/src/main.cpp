#include <exception>
#include <iostream>

#include <xprog/cli.h>
#include <xprog/errors.h>
#include <xprog/package.h>

#ifndef XPROG_VERSION
#define XPROG_VERSION "0.1.0"
#endif

int main(int argc, char* argv[])
{
    try {
        const auto options = xprog::cli::parse(argc, argv);
        if (options.show_help) {
            xprog::cli::print_usage(argc > 0 ? argv[0] : "xprog");
            return 0;
        }
        if (options.show_version) {
            std::cout << "xprog " << XPROG_VERSION
                      << " (X Tools Program Packager for Z80)\n";
            return 0;
        }
        xprog::run(options, std::cout);
        return 0;
    } catch (const xprog::usage_error& e) {
        std::cerr << "xprog: error: " << e.what() << "\n"
                  << "Try 'xprog --help' for usage.\n";
        return 1;
    } catch (const xprog::error& e) {
        std::cerr << "xprog: error: " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "xprog: internal error: " << e.what() << "\n";
        return 2;
    }
}
