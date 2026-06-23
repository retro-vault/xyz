// map_emitter.h
//
// linker map writer
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#ifndef XLINK_MAP_EMITTER_HPP
#define XLINK_MAP_EMITTER_HPP

#include <filesystem>
#include <iosfwd>

#include <xld/link_context.h>

namespace xld {

    class map_emitter {
    public:
        static void emit(std::ostream& out, const link_context& ctx);
        static void emit(const std::filesystem::path& path,
                         const link_context& ctx);
    };

} // namespace xld

#endif // XLINK_MAP_EMITTER_HPP
