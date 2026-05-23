//
// debug sidecar emitter interface
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
#ifndef XLINK_DEBUG_EMITTER_HPP
#define XLINK_DEBUG_EMITTER_HPP

#include <filesystem>

#include <xlink/link_context.hpp>

namespace xlink {

    class debug_emitter {
    public:
        virtual ~debug_emitter() = default;

        virtual void emit(const std::filesystem::path& path,
                          const std::filesystem::path& image_path,
                          const link_context& ctx) const = 0;
    };

} // namespace xlink

#endif // XLINK_DEBUG_EMITTER_HPP
