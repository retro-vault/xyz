//
// SDCC runtime directory helpers
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
#ifndef XLINK_RUNTIME_HPP
#define XLINK_RUNTIME_HPP

#include <xld/cli.h>

namespace xld {

    class runtime {
    public:
        static void apply_sdcc_runtime(cli_options& opts);
    };

} // namespace xld

#endif // XLINK_RUNTIME_HPP
