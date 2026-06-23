//
// SDCC .cdb emitter
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
#ifndef XLINK_CDB_EMITTER_HPP
#define XLINK_CDB_EMITTER_HPP

#include <xld/debug_emitter.h>

namespace xld {

    class cdb_emitter final : public debug_emitter {
    public:
        void emit(const std::filesystem::path& path,
                  const std::filesystem::path& image_path,
                  const link_context& ctx) const override;
    };

} // namespace xld

#endif // XLINK_CDB_EMITTER_HPP
