// branch_relaxer.h
//
// Link-time branch canonicalization:
// - shrink local in-area JP to JR when safe
// - promote out-of-range short branches back to long forms after final
//   placement decisions (including reserved BIN holes)
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#ifndef XLINK_BRANCH_RELAXER_HPP
#define XLINK_BRANCH_RELAXER_HPP

#include <xld/link_context.h>

namespace xld {

    class branch_relaxer {
    public:
        static void relax(link_context& ctx);
    };

} // namespace xld

#endif // XLINK_BRANCH_RELAXER_HPP
