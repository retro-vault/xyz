//
// SDCC runtime directory helpers
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
#ifndef XLINK_RUNTIME_HPP
#define XLINK_RUNTIME_HPP

#include <xlink/cli.hpp>

namespace xlink {

    class runtime {
    public:
        static void apply_sdcc_runtime(cli_options& opts);
    };

} // namespace xlink

#endif // XLINK_RUNTIME_HPP
