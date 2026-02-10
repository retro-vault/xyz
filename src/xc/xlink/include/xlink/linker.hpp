// linker.hpp
//
// pipeline orchestrator
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
// 2021-07-28   tstih
#ifndef XLINK_LINKER_HPP
#define XLINK_LINKER_HPP

#include <xlink/link_context.hpp>
#include <xlink/cli.hpp>

namespace xlink {

    class linker {
    public:
        // Run the full linking pipeline.
        static void link(link_context& ctx, const cli_options& opts);

    private:
        static void load_inputs(link_context& ctx, const cli_options& opts);
        static void resolve_libraries(link_context& ctx,
                                      const cli_options& opts);
        static void resolve_symbols(link_context& ctx);
        static void place_areas(link_context& ctx);
        static void relocate(link_context& ctx);
        static void find_entry_point(link_context& ctx);
    };

} // namespace xlink

#endif // XLINK_LINKER_HPP
