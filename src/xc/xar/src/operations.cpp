// operations.cpp
//
// Archive operations for xar: add/replace, list, extract, delete.
// All operations go through libxbfd so both SDCC text-index and GNU ar
// formats are supported transparently.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <xar/operations.h>
#include <xbfd/xbfd.h>

namespace xar {

    namespace {

        // Load raw bytes from a file.
        static std::string read_file(const std::filesystem::path& p)
        {
            std::ifstream f(p, std::ios::binary);
            if (!f.is_open())
                throw std::runtime_error("cannot open: " + p.string());
            return std::string((std::istreambuf_iterator<char>(f)),
                               std::istreambuf_iterator<char>());
        }

        // Return true if name matches any member name in the list.
        static bool in_list(const std::string& name,
                             const std::vector<std::string>& list)
        {
            for (const auto& m : list)
                if (m == name || std::filesystem::path(m).filename() == name)
                    return true;
            return false;
        }

        // Load existing archive members; returns empty if archive does not exist.
        static std::vector<bfd::archive_member> load_existing(
            const std::string& archive_path)
        {
            if (!std::filesystem::exists(archive_path))
                return {};
            try {
                auto arc = bfd::bfd::open_r(archive_path);
                if (!arc->check_format(bfd::format::archive))
                    throw std::runtime_error("not an archive: " + archive_path);
                return arc->members();
            } catch (const bfd::bfd_error&) {
                return {};
            }
        }

        // Write an archive from a member list.
        static void write_archive(const std::string& path,
                                   archive_mode mode,
                                   const std::vector<bfd::archive_member>& members)
        {
            bfd::flavour fmt = (mode == archive_mode::gnu)
                               ? bfd::flavour::ar_binary
                               : bfd::flavour::ar_text;
            auto arc = bfd::bfd::create_archive(path, fmt);
            for (const auto& m : members)
                arc->add_member(m);
            arc->close();
        }

    } // anonymous namespace

    // -------------------------------------------------------------------------
    // op_add — add or replace members
    // -------------------------------------------------------------------------

    void op_add(const cli_options& opts)
    {
        auto existing = load_existing(opts.archive);

        // Build a map of existing member name → index for replacement.
        std::unordered_map<std::string, size_t> idx_map;
        for (size_t i = 0; i < existing.size(); ++i)
            idx_map[existing[i].name] = i;

        for (const auto& member_path : opts.members) {
            std::filesystem::path p(member_path);
            std::string name = p.filename().string();

            bfd::archive_member m;
            m.name = name;

            if (opts.mode == archive_mode::gnu) {
                // ar binary: embed raw bytes.
                m.data = read_file(p);
                m.path = p;
            } else {
                // text-index: store path reference.
                m.path = p;
            }

            auto it = idx_map.find(name);
            if (it != idx_map.end()) {
                existing[it->second] = std::move(m);
                if (opts.verbose)
                    std::cout << "r " << name << "\n";
            } else {
                existing.push_back(std::move(m));
                if (opts.verbose)
                    std::cout << "a " << name << "\n";
            }
        }

        write_archive(opts.archive, opts.mode, existing);
    }

    // -------------------------------------------------------------------------
    // op_list — list archive contents
    // -------------------------------------------------------------------------

    void op_list(const cli_options& opts)
    {
        auto arc = bfd::bfd::open_r(opts.archive);
        if (!arc->check_format(bfd::format::archive))
            throw std::runtime_error("not an archive: " + opts.archive);

        for (const auto& m : arc->members()) {
            if (opts.verbose) {
                size_t sz = m.data.has_value() ? m.data->size() : 0;
                if (!m.data.has_value() && !m.path.empty()
                    && std::filesystem::exists(m.path))
                    sz = std::filesystem::file_size(m.path);
                std::cout << std::setw(8) << sz << " " << m.name << "\n";
            } else {
                std::cout << m.name << "\n";
            }
        }
    }

    // -------------------------------------------------------------------------
    // op_extract — extract members to current directory
    // -------------------------------------------------------------------------

    void op_extract(const cli_options& opts)
    {
        auto arc = bfd::bfd::open_r(opts.archive);
        if (!arc->check_format(bfd::format::archive))
            throw std::runtime_error("not an archive: " + opts.archive);

        bool all = opts.members.empty();

        for (const auto& m : arc->members()) {
            if (!all && !in_list(m.name, opts.members))
                continue;

            std::string out_name = std::filesystem::path(m.name).filename().string();

            if (m.data.has_value()) {
                std::ofstream out(out_name, std::ios::binary | std::ios::trunc);
                if (!out.is_open())
                    throw std::runtime_error("cannot write: " + out_name);
                out.write(m.data->data(),
                          static_cast<std::streamsize>(m.data->size()));
            } else if (!m.path.empty() && std::filesystem::exists(m.path)) {
                std::filesystem::copy_file(m.path, out_name,
                    std::filesystem::copy_options::overwrite_existing);
            }

            if (opts.verbose)
                std::cout << "x " << out_name << "\n";
        }
    }

    // -------------------------------------------------------------------------
    // op_delete — remove named members
    // -------------------------------------------------------------------------

    void op_delete(const cli_options& opts)
    {
        if (opts.members.empty()) return;

        auto existing = load_existing(opts.archive);
        std::vector<bfd::archive_member> kept;
        kept.reserve(existing.size());

        for (auto& m : existing) {
            if (in_list(m.name, opts.members)) {
                if (opts.verbose)
                    std::cout << "d " << m.name << "\n";
                continue;
            }
            kept.push_back(std::move(m));
        }

        write_archive(opts.archive, opts.mode, kept);
    }

} // namespace xar
