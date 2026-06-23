// debug_info/writer.cpp — xbfd::writer (multicast di_writer), debug_info methods,
//                          and debug_reader backward-compat helpers.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <memory>
#include <xbfd/xbfd.h>

namespace xbfd {

// =========================================================================
// debug_info member functions
// =========================================================================

const debug_function* debug_info::function_at(uint32_t a) const {
    for (const auto& f : functions) if (a >= f.start && a < f.end) return &f;
    return nullptr;
}
const debug_line* debug_info::line_at(uint32_t a) const {
    const debug_line* b = nullptr;
    for (const auto& e : lines)
        if (e.address <= a && (!b || e.address > b->address)) b = &e;
    return b;
}
const debug_source_file* debug_info::file_by_id(uint32_t id) const {
    for (const auto& f : files) if (f.id == id) return &f;
    return nullptr;
}

// =========================================================================
// writer: add_cdb / add_dwarf convenience methods
// =========================================================================

writer& writer::add_cdb(std::ostream& o, const std::string& src, const std::string& adb) {
    return add(std::make_unique<cdb>(o, src, adb));
}
writer& writer::add_dwarf(std::ostream& o, const std::string& src) {
    return add(std::make_unique<dwarf2_writer>(o, src));
}

// =========================================================================
// debug_reader: backward-compat static helpers
// =========================================================================

std::optional<debug_info> debug_reader::read_cdb(const std::string& path) {
    return cdb{}.read(path);
}
std::optional<debug_info> debug_reader::read_map(const std::string& path, debug_info base) {
    return map_reader{}.read(path, std::move(base));
}
debug_info debug_reader::merge(debug_info base, const debug_info& supplement) {
    return map_reader::merge(std::move(base), supplement);
}

} // namespace xbfd
