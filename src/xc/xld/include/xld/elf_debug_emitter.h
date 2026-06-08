//
// GNU ELF + DWARF2 debug emitter
//
// MIT License (see: LICENSE)
//
#ifndef XLINK_ELF_DEBUG_EMITTER_HPP
#define XLINK_ELF_DEBUG_EMITTER_HPP

#include <xld/debug_emitter.h>

namespace xld {

    class elf_debug_emitter final : public debug_emitter {
    public:
        void emit(const std::filesystem::path& path,
                  const std::filesystem::path& image_path,
                  const link_context& ctx) const override;
    };

} // namespace xld

#endif // XLINK_ELF_DEBUG_EMITTER_HPP
