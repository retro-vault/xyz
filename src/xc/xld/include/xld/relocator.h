// relocator.h
//
// relocation processor
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
// 2021-07-28   tstih
#ifndef XLINK_RELOCATOR_HPP
#define XLINK_RELOCATOR_HPP

#include <xld/link_context.h>

namespace xld {

    class relocator {
    public:
        // Process all text records: copy data into code_buffer,
        // apply relocations, build reloc_table.
        static void relocate(link_context& ctx);
    };

} // namespace xld

#endif // XLINK_RELOCATOR_HPP
