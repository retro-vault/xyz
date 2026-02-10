// relocator.hpp
//
// relocation processor
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
// 2021-07-28   tstih
#ifndef XLINK_RELOCATOR_HPP
#define XLINK_RELOCATOR_HPP

#include <xlink/link_context.hpp>

namespace xlink {

    class relocator {
    public:
        // Process all text records: copy data into code_buffer,
        // apply relocations, build reloc_table.
        static void relocate(link_context& ctx);
    };

} // namespace xlink

#endif // XLINK_RELOCATOR_HPP
