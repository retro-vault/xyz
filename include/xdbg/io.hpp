/*
 * Declares the xdbg document input and output helpers used to read
 * and write debugger metadata streams and files.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */

#ifndef XDBG_IO_HPP
#define XDBG_IO_HPP

#include <filesystem>
#include <iosfwd>

#include <xdbg/model.hpp>

namespace xdbg {

    // Read an xdbg document from a character stream.
    //
    // Parameters:
    //      input       - Stream containing serialized xdbg data.
    //
    // Returns:
    //      Parsed xdbg document.
    document read(std::istream& input);

    // Read an xdbg document from a file on disk.
    //
    // Parameters:
    //      path        - Path to the xdbg document file.
    //
    // Returns:
    //      Parsed xdbg document.
    document read_file(const std::filesystem::path& path);

    // Write an xdbg document to a character stream.
    //
    // Parameters:
    //      output      - Destination stream.
    //      doc         - Document to serialize.
    //
    // Returns:
    //      Nothing.
    void write(std::ostream& output, const document& doc);

    // Write an xdbg document to a file on disk.
    //
    // Parameters:
    //      path        - Output file path.
    //      doc         - Document to serialize.
    //
    // Returns:
    //      Nothing.
    void write_file(const std::filesystem::path& path, const document& doc);

} // namespace xdbg

#endif // XDBG_IO_HPP
