// operations.hpp
//
// High-level archive operations for xar.  Each function maps to one of
// the supported operations: add/replace, list, extract, delete.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#ifndef XAR_OPERATIONS_HPP
#define XAR_OPERATIONS_HPP

#include <xar/cli.hpp>

namespace xar {

    //
    // Add or replace members in archive.
    // Creates the archive if it does not exist.
    //
    void op_add(const cli_options& opts);

    //
    // List archive contents.
    //
    void op_list(const cli_options& opts);

    //
    // Extract one or more members.
    // If opts.members is empty, extract all.
    //
    void op_extract(const cli_options& opts);

    //
    // Delete named members from the archive.
    //
    void op_delete(const cli_options& opts);

} // namespace xar

#endif // XAR_OPERATIONS_HPP
