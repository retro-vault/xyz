// link_context.h
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
#include <optional>
#include <string>
#include <vector>

#include <xld/types.h>
#include <xld/module.h>

namespace xld {

    // Output relocation table entry.
    struct output_reloc {
        uint16_t offset;    // offset into code_buffer
        uint8_t size;       // 1 = byte, 2 = word
        uint8_t pad;        // bit0: byte relocation patches MSB
    };

    class link_context {
    public:
        // Loaded modules.
        std::vector<std::shared_ptr<module>> modules;

        // Reserved address ranges (holes).
        std::vector<address_range> holes;
        std::map<std::string, uint16_t> area_bases;
        std::vector<std::string> area_order;
        std::optional<address_range> output_range;
        output_format format = output_format::xl;

        // Entry point symbol name.
        std::string entry_name = "_main";

        // Global symbol table: name -> (module, symbol index).
        std::map<std::string, std::pair<module*, int>> global_symbols;
        std::map<std::string, uint16_t> linker_symbols;

        // Output data.
        std::vector<uint8_t> code_buffer;
        std::vector<output_reloc> reloc_table;
        uint16_t entry_point = 0;
        uint16_t code_size = 0;

        // Verbose flag.
        bool verbose = false;
        bool print_map = false;
    };

} // namespace xld

#endif // XLINK_LINK_CONTEXT_HPP
