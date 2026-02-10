// area_placer.hpp
//
// area placement algorithm
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
// 2021-07-28   tstih
#ifndef XLINK_AREA_PLACER_HPP
#define XLINK_AREA_PLACER_HPP

#include <cstdint>
#include <vector>

#include <xlink/types.hpp>
#include <xlink/link_context.hpp>

namespace xlink {

    class area_placer {
    public:
        // Place all areas in all modules, updating placed_addr.
        static void place(link_context& ctx);

        // Find next free address >= cursor that can fit `size` bytes
        // without overlapping any hole.
        static uint16_t next_free_address(
            uint16_t cursor, uint16_t size,
            const std::vector<address_range>& holes);
    };

} // namespace xlink

#endif // XLINK_AREA_PLACER_HPP
