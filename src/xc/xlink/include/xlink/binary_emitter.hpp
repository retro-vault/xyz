// binary_emitter.hpp
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

#include <xlink/link_context.hpp>

namespace xlink {

    class binary_emitter {
    public:
        // Write the output file: header + reloc table + code.
        static void emit(const std::filesystem::path& path,
                         const link_context& ctx);
    };

} // namespace xlink

#endif // XLINK_BINARY_EMITTER_HPP
