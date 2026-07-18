// elf_emitter.h
//
// primary linked ELF output writer
//
// MIT License (see: LICENSE)
#ifndef XLINK_ELF_EMITTER_HPP
#define XLINK_ELF_EMITTER_HPP

#include <filesystem>

#include <xld/link_context.h>

namespace xld {

    class elf_emitter {
    public:
        static void emit(const std::filesystem::path& path,
                         const link_context& ctx,
                         bool include_debug = false);
    };

} // namespace xld

#endif // XLINK_ELF_EMITTER_HPP
