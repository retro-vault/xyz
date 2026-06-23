// binary_emitter.h
//
// output file writer
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
// 2021-07-28   tstih
#ifndef XLINK_BINARY_EMITTER_HPP
#define XLINK_BINARY_EMITTER_HPP

#include <filesystem>

#include <xld/link_context.h>

namespace xld {

    class binary_emitter {
    public:
        // Write the output file: header + reloc table + code.
        static void emit(const std::filesystem::path& path,
                         const link_context& ctx);
    };

} // namespace xld

#endif // XLINK_BINARY_EMITTER_HPP
