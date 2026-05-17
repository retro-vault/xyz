/*
 * Defines the xdbg document model used to exchange source, symbol,
 * function, and variable metadata with debugging tools.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */

#ifndef XDBG_MODEL_HPP
#define XDBG_MODEL_HPP

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace xdbg {

    // Source language associated with a symbol or source file entry.
    enum class language_kind {
        unknown,    // Language is unknown or not specified.
        c,          // C source language.
        cxx,        // C++ source language.
        assembly    // Assembly language source.
    };

    // High-level kind of symbol stored in the document.
    enum class symbol_kind {
        unknown,     // Symbol kind is unknown.
        function,    // Function or procedure symbol.
        global,      // Global-scope symbol.
        local,       // Local-scope symbol.
        parameter,   // Function parameter symbol.
        label,       // Code or data label.
        section,     // Section or segment symbol.
        type,        // Type symbol.
        constant,    // Constant value symbol.
        object       // Data object symbol.
    };

    // Physical storage location used for a variable.
    enum class storage_kind {
        unknown,         // Storage location is unknown.
        address,         // Variable lives at an absolute address.
        stack,           // Variable lives on the stack.
        register_name,   // Variable lives in a named register.
        register_pair,   // Variable lives in a register pair.
        frame_relative   // Variable lives at a frame-relative offset.
    };

    // Source file record referenced by symbols and line mappings.
    struct source_file {
        uint32_t id = 0;                                 // Stable file identifier within the document.
        std::string path;                                // Source path string as stored by the producer.
        language_kind language = language_kind::unknown; // Source language of the file.
    };

    // Named symbol with optional source and type metadata.
    struct symbol {
        std::string name;                                // Symbol name.
        symbol_kind kind = symbol_kind::unknown;         // Symbol classification.
        uint32_t address = 0;                            // Symbol address in the image.
        std::optional<uint32_t> size;                    // Optional symbol size in bytes.
        std::optional<uint32_t> file_id;                 // Optional source file identifier.
        std::optional<uint32_t> line;                    // Optional source line number.
        std::optional<uint32_t> column;                  // Optional source column number.
        std::optional<std::string> type_name;            // Optional type name string.
        std::optional<std::string> parent_name;          // Optional parent symbol or scope name.
        language_kind language = language_kind::unknown; // Source language associated with the symbol.
    };

    /*
     * Function range with optional source and type metadata.
     */
    struct function {
        std::string name;                              /* Function name. */
        uint32_t start_address = 0;                    /* First address covered by the function. */
        uint32_t end_address = 0;                      /* Address immediately after the function or final covered address. */
        std::optional<uint32_t> file_id;               /* Optional source file identifier. */
        std::optional<uint32_t> line;                  /* Optional starting source line number. */
        std::optional<uint32_t> column;                /* Optional starting source column number. */
        std::optional<std::string> return_type;        /* Optional return type name. */
        language_kind language = language_kind::unknown; /* Source language associated with the function. */
    };

    /*
     * Mapping from machine address back to a source location.
     */
    struct line_entry {
        uint32_t address = 0;   /* Machine address for this mapping. */
        uint32_t file_id = 0;   /* Referenced source file identifier. */
        uint32_t line = 0;      /* Source line number. */
        uint32_t column = 0;    /* Source column number. */
    };

    /*
     * Variable description with storage and scope metadata.
     */
    struct variable {
        std::string name;                              /* Variable name. */
        symbol_kind kind = symbol_kind::local;         /* Variable classification. */
        std::optional<std::string> parent_name;        /* Optional parent scope or function name. */
        storage_kind storage = storage_kind::unknown;  /* Storage location kind. */
        std::optional<uint32_t> address;               /* Optional absolute address. */
        std::optional<int32_t> offset;                 /* Optional stack or frame offset. */
        std::optional<std::string> register_name;      /* Optional register or register pair name. */
        std::optional<std::string> type_name;          /* Optional type name string. */
        std::optional<uint32_t> start_address;         /* Optional start address of the variable lifetime. */
        std::optional<uint32_t> end_address;           /* Optional end address of the variable lifetime. */
        std::optional<uint32_t> file_id;               /* Optional source file identifier. */
        std::optional<uint32_t> line;                  /* Optional source line number. */
        std::optional<uint32_t> column;                /* Optional source column number. */
        language_kind language = language_kind::unknown; /* Source language associated with the variable. */
    };

    /*
     * Root xdbg document containing all exported debug metadata.
     */
    struct document {
        uint32_t version = 1;                          /* Document format version. */
        std::optional<std::string> image_path;         /* Optional original binary or image path. */
        std::optional<uint32_t> entry_address;         /* Optional program entry address. */
        std::vector<source_file> files;                /* Source file table. */
        std::vector<symbol> symbols;                   /* Symbol table. */
        std::vector<function> functions;               /* Function table. */
        std::vector<line_entry> lines;                 /* Source line mapping table. */
        std::vector<variable> variables;               /* Variable metadata table. */
    };

} // namespace xdbg

#endif // XDBG_MODEL_HPP
