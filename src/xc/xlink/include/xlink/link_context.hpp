// link_context.hpp
//
// global linker state
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
// 2021-07-28   tstih
#ifndef XLINK_LINK_CONTEXT_HPP
#define XLINK_LINK_CONTEXT_HPP

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <xlink/types.hpp>
#include <xlink/module.hpp>

namespace xlink {

    // Output relocation table entry.
    struct output_reloc {
        uint16_t offset;    // offset into code_buffer
        uint8_t size;       // 1 = byte, 2 = word
        uint8_t pad;        // reserved, 0
    };

    class link_context {
    public:
        // Loaded modules.
        std::vector<std::shared_ptr<module>> modules;

        // Reserved address ranges (holes).
        std::vector<address_range> holes;

        // Entry point symbol name.
        std::string entry_name = "_main";

        // Global symbol table: name -> (module, symbol index).
        std::map<std::string, std::pair<module*, int>> global_symbols;

        // Output data.
        std::vector<uint8_t> code_buffer;
        std::vector<output_reloc> reloc_table;
        uint16_t entry_point = 0;
        uint16_t code_size = 0;

        // Verbose flag.
        bool verbose = false;
        bool print_map = false;
    };

} // namespace xlink

#endif // XLINK_LINK_CONTEXT_HPP
