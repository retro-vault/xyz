# Minimal switches

```
-o <file>	Write output to <file> instead of the default (stdin→stdout)
-c	Compile only; don’t link or invoke any later stages
-g	Generate debug symbols (for DWARF/ELF output, as you grow)
-I<dir>	Add <dir> to the include‑path search list
-D<macro>[=val]	Predefine a constant or symbol for the assembler (for equ‑like macros)
-v/--verbose	Print out extra info about what the assembler is doing
--help	Show usage and exit
--version	Print version string and exit
```

# How to implement

```cpp
#include <getopt.h>
#include <iostream>
#include <string>
#include <vector>

struct options {
    std::vector<std::string> input_files;  // <-- collect these
    std::string output_file;
    bool compile_only = false;
    bool debug = false;
    std::vector<std::string> include_dirs;
    std::vector<std::string> defines;
    bool verbose = false;
};

options parse_args(int argc, char** argv) {
    options opts;
    const struct option longopts[] = {
        {"help",     no_argument,       0, 'h'},
        {"version",  no_argument,       0, 'V'},
        {"output",   required_argument, 0, 'o'},
        {"compile-only", no_argument,   0, 'c'},
        {"debug",    no_argument,       0, 'g'},
        {"include",  required_argument, 0, 'I'},
        {"define",   required_argument, 0, 'D'},
        {"verbose",  no_argument,       0, 'v'},
        {0,0,0,0}
    };
    int c;
    while ((c = getopt_long(argc, argv, "hVo:c gI:D:v", longopts, nullptr)) != -1) {
        switch (c) {
        case 'h':
            std::cout << "Usage: xas [options] file.s\n";
            std::exit(0);
        case 'V':
            std::cout << "xas version 1.0\n";
            std::exit(0);
        case 'o':
            opts.output_file = optarg;
            break;
        case 'c':
            opts.compile_only = true;
            break;
        case 'g':
            opts.debug = true;
            break;
        case 'I':
            opts.include_dirs.push_back(optarg);
            break;
        case 'D':
            opts.defines.push_back(optarg);
            break;
        case 'v':
            opts.verbose = true;
            break;
        default:
            std::cerr << "Unknown option\n";
            std::exit(1);
        }
    }

    // All options processed: everything left are input files
    for (int i = optind; i < argc; ++i) {
        opts.input_files.emplace_back(argv[i]);
    }

    if (opts.input_files.empty()) {
        std::cerr << "Error: no input files specified\n";
        std::exit(1);
    }

    return opts;

    return opts;
}
```

```cpp
int main(int argc, char** argv) {
    auto opts = parse_args(argc, argv);

    // Now opts.input_files is your list of .s (or other) files
    for (auto &infile : opts.input_files) {
        std::cout << "Assembling: " << infile << "\n";
        // 1) read infile
        // 2) assemble
        // 3) write object to opts.output_file or derive .o per input
    }

    return 0;
}

```
