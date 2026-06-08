//
// lscript.cpp — shared linker-script dispatcher.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include <xbfd/lscript.h>

namespace xbfd {

std::unique_ptr<lscript> lscript::open(const std::filesystem::path& path,
                                       lscript_mode mode)
{
    switch (mode) {
    case lscript_mode::gnu:
        return gnu_lscript::read(path);
    case lscript_mode::sdcc:
    default:
        return sdcc_lscript::read(path);
    }
}

} // namespace xbfd
