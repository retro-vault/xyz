// main.cpp
//
// xar — Z80 archive tool for the xyz toolchain.
// Creates, modifies, lists, and extracts .lib / .a archives.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <iostream>
#include <stdexcept>

#include <xar/cli.h>
#include <xar/operations.h>

int main(int argc, char** argv)
{
    try {
        auto opts = xar::parse_args(argc, argv);

        switch (opts.op) {
            case xar::operation::add:
            case xar::operation::create:
                xar::op_add(opts);
                break;
            case xar::operation::list:
                xar::op_list(opts);
                break;
            case xar::operation::extract:
                xar::op_extract(opts);
                break;
            case xar::operation::remove:
                xar::op_delete(opts);
                break;
        }
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "xar: error: " << e.what() << "\n";
        return 1;
    }
}
