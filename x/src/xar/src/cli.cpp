// cli.cpp
//
// xar command-line argument parser.
// Supports both traditional positional flags ("rcs archive.lib file.rel")
// and separate flag groups ("--mode=gnu r archive.a file.o").
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include <xar/cli.h>

namespace xar {

#ifndef XTOOLS_VERSION
#define XTOOLS_VERSION "0.1.0"
#endif

    namespace {

        std::string normalize_cli_path(std::string path)
        {
#ifdef _WIN32
            // Response files bypass the MSYS argv path rewrite, so convert
            // /c/... style paths into native C:/... form before opening them.
            if (path.size() >= 3
                && path[0] == '/'
                && std::isalpha(static_cast<unsigned char>(path[1]))
                && path[2] == '/') {
                path = std::string(
                    1,
                    static_cast<char>(
                        std::toupper(static_cast<unsigned char>(path[1]))))
                    + ":" + path.substr(2);
            }
#endif
            return path;
        }

        void append_members_from_response_file(
            const std::string& path,
            std::vector<std::string>& members)
        {
            std::ifstream file(normalize_cli_path(path));
            if (!file.is_open())
                throw std::runtime_error("cannot open response file: " + path);

            std::string line;
            while (std::getline(file, line)) {
                if (!line.empty() && line.back() == '\r')
                    line.pop_back();
                if (!line.empty())
                    members.push_back(normalize_cli_path(std::move(line)));
            }
        }

    } // anonymous namespace

    [[noreturn]] void print_usage_and_exit(const char* prog, int code)
    {
        std::cerr
            << "Usage: " << prog << " [--mode=sdcc|gnu] <operation> archive [members...]\n"
            << "\n"
            << "X Tools Archiver (xar) — Z80 archive manager\n"
            << "\n"
            << "operations:\n"
            << "  r   Add or replace members\n"
            << "  t   List archive contents\n"
            << "  x   Extract members (all if none specified)\n"
            << "  d   Delete members\n"
            << "\n"
            << "modifiers (combine with operation letter):\n"
            << "  c   Create archive (suppress warning)\n"
            << "  v   Verbose\n"
            << "  s   Write symbol index (reserved, no-op)\n"
            << "\n"
            << "options:\n"
            << "  --mode=sdcc   Text-index .lib format (default)\n"
            << "  --mode=gnu    GNU ar binary format\n"
            << "  @file         Read member paths from a response file\n"
            << "  @@file        Treat a leading '@' as part of a literal member path\n"
            << "  --version     Print version\n"
            << "  -h, --help    Show this help\n"
            << "\n"
            << "examples:\n"
            << "  xar rcs mylib.lib foo.rel bar.rel\n"
            << "  xar --mode=gnu rc mylib.a foo.o bar.o\n"
            << "  xar rcs mylib.lib @members.rsp\n"
            << "  xar t mylib.lib\n"
            << "  xar x mylib.lib foo.rel\n"
            << "  xar d mylib.lib old.rel\n";
        std::exit(code);
    }

    cli_options parse_args(int argc, char** argv)
    {
        cli_options opts;
        const char* prog = argc > 0 ? argv[0] : "xar";
        int i = 1;

        if (argc < 2)
            print_usage_and_exit(prog, 1);

        // Process --options first.
        for (; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "-h" || arg == "--help") print_usage_and_exit(prog, 0);
            if (arg == "--version") {
                std::cout << "xar " << XTOOLS_VERSION
                          << " (X Tools Archiver for Z80)\n";
                std::exit(0);
            }
            if (arg == "--mode=sdcc") { opts.mode = archive_mode::sdcc; continue; }
            if (arg == "--mode=gnu")  { opts.mode = archive_mode::gnu;  continue; }
            if (arg[0] != '-') break; // positional argument
            throw std::runtime_error("unknown option: " + arg);
        }

        if (i >= argc)
            print_usage_and_exit(prog, 1);

        // Parse operation string (e.g. "rcs", "t", "xv").
        std::string opstr = argv[i++];
        operation op_found = operation::add;
        bool op_set = false;

        for (char c : opstr) {
            switch (c) {
                case 'r': op_found = operation::add;     op_set = true; break;
                case 't': op_found = operation::list;    op_set = true; break;
                case 'x': op_found = operation::extract; op_set = true; break;
                case 'd': op_found = operation::remove;  op_set = true; break;
                case 'c': opts.create  = true; break;
                case 'v': opts.verbose = true; break;
                case 's': break; // symbol index — reserved
                default:
                    throw std::runtime_error(
                        std::string("unknown operation modifier: ") + c);
            }
        }
        if (!op_set)
            throw std::runtime_error("no operation in: " + opstr);

        opts.op = op_found;

        if (i >= argc)
            throw std::runtime_error("no archive name specified");

        opts.archive = normalize_cli_path(argv[i++]);

        while (i < argc) {
            std::string arg = argv[i++];
            if (arg.rfind("@@", 0) == 0) {
                opts.members.push_back(normalize_cli_path(arg.substr(1)));
                continue;
            }
            if (!arg.empty() && arg[0] == '@') {
                if (arg.size() == 1)
                    throw std::runtime_error("empty response file argument");
                append_members_from_response_file(arg.substr(1), opts.members);
                continue;
            }
            opts.members.push_back(normalize_cli_path(std::move(arg)));
        }

        return opts;
    }

} // namespace xar
