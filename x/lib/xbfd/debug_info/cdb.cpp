// debug_info/cdb.cpp — xbfd::cdb: SDCC CDB reader (di_reader) + streaming emitter (di_writer).
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <ostream>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <xbfd/xbfd.h>

namespace xbfd {

// =========================================================================
// CDB type helpers
// =========================================================================

namespace {

constexpr uint32_t k_unset_address = 0xFFFFu;

std::optional<std::vector<std::string>> read_lines(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open())
        return std::nullopt;

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line))
        lines.push_back(line);
    return lines;
}

std::string source_label_name(const std::string& source_file) {
    std::string name = std::filesystem::path(source_file).filename().string();
    if (name.empty())
        name = "source";

    for (char& ch : name) {
        const bool keep = (ch >= 'a' && ch <= 'z')
            || (ch >= 'A' && ch <= 'Z')
            || (ch >= '0' && ch <= '9')
            || ch == '_'
            || ch == '.';
        if (!keep)
            ch = '_';
    }
    return name;
}

std::string_view trim(std::string_view str) {
    const auto first = str.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos)
        return {};
    const auto last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
}

std::vector<std::string_view> split(std::string_view str, char delim) {
    std::vector<std::string_view> parts;
    std::size_t start = 0;
    while (start <= str.size()) {
        auto end = str.find(delim, start);
        if (end == std::string_view::npos)
            end = str.size();
        parts.push_back(str.substr(start, end - start));
        start = end + 1;
    }
    return parts;
}

std::optional<calling_convention> parse_abi_field(
    const std::vector<std::string_view>& fields)
{
    for (auto field : fields) {
        field = trim(field);
        constexpr std::string_view prefix = "ABI=";
        if (field.substr(0, prefix.size()) != prefix)
            continue;
        return parse_calling_convention(trim(field.substr(prefix.size())));
    }
    return std::nullopt;
}

std::string strip_scope_qualifier(std::string_view name) {
    if (const auto dot = name.find('.'); dot != std::string_view::npos)
        return std::string(name.substr(dot + 1));
    return std::string(name);
}

debug_lang lang_from(const std::string& path) {
    const auto ext = std::filesystem::path(path).extension().string();
    if (ext == ".c" || ext == ".C")
        return debug_lang::c;
    if (ext == ".s" || ext == ".S" || ext == ".asm")
        return debug_lang::assembly;
    return debug_lang::unknown;
}

var_storage storage_from_code(char code) {
    switch (code) {
    case 'B': return var_storage::stack;
    case 'R': return var_storage::reg;
    case 'I':
    case 'E':
    case 'A': return var_storage::external;
    default:  return var_storage::unknown;
    }
}

static std::string cdb_base(const type_ref& t) {
    switch (t.base) {
    case type_ref::kind::void_:    return "SV:S";  case type_ref::kind::char_:    return "SC:S";
    case type_ref::kind::uchar:    return "SC:U";  case type_ref::kind::short_:   return "SI:S";
    case type_ref::kind::ushort:   return "SI:U";  case type_ref::kind::int_:     return "SI:S";
    case type_ref::kind::uint_:    return "SI:U";  case type_ref::kind::long_:    return "SL:S";
    case type_ref::kind::ulong:    return "SL:U";  case type_ref::kind::llong:    return "SQ:S";
    case type_ref::kind::ullong:   return "SQ:U";  case type_ref::kind::float_:   return "SF:S";
    case type_ref::kind::double_:  return "SF:S";  case type_ref::kind::pointer:  return "DP,SV:S";
    case type_ref::kind::array:    return "DA" + std::to_string(t.array_count) + "d,SC:U";
    case type_ref::kind::function: return "DF,SV:S";
    case type_ref::kind::struct_:  return "ST" + (t.tag.empty() ? "?" : t.tag) + ":S";
    case type_ref::kind::union_:   return "SU" + (t.tag.empty() ? "?" : t.tag) + ":S";
    default: return "SI:S";
    }
}

static std::string cdb_type(const type_ref& t) {
    return "{" + std::to_string(t.size_bytes > 0 ? t.size_bytes : 2) + "}" + cdb_base(t);
}

static char sc(var_storage s) {
    switch (s) {
    case var_storage::stack:    return 'B';
    case var_storage::reg:      return 'R';
    case var_storage::external: return 'E';
    default:                    return 'A';
    }
}

struct cdb_module_state {
    std::string name;
    std::string file;
    std::unordered_map<std::string, std::size_t> function_indexes;
    std::vector<debug_line> source_lines;
    std::vector<std::pair<int, uint32_t>> asm_lines;
};

class cdb_parser {
public:
    std::optional<debug_info> parse(const std::string& path) {
        auto lines = read_lines(path);
        if (!lines)
            return std::nullopt;

        reset();
        for (const auto& raw : *lines) {
            const auto line = trim(raw);
            if (!line.empty())
                parse_line(line);
        }
        finalize();
        return info_;
    }

private:
    debug_info info_;
    std::vector<cdb_module_state> modules_;
    std::unordered_map<std::string, std::size_t> module_indexes_;
    std::unordered_map<std::string, uint32_t> file_ids_;
    std::string current_module_;
    uint32_t next_file_id_ = 1;

    void reset() {
        info_ = {};
        modules_.clear();
        module_indexes_.clear();
        file_ids_.clear();
        current_module_.clear();
        next_file_id_ = 1;
    }

    uint32_t intern_file(const std::string& path) {
        const auto it = file_ids_.find(path);
        if (it != file_ids_.end())
            return it->second;

        const auto id = next_file_id_++;
        file_ids_[path] = id;
        info_.files.push_back({id, path, lang_from(path)});
        return id;
    }

    cdb_module_state& ensure_module(const std::string& name) {
        const auto it = module_indexes_.find(name);
        if (it != module_indexes_.end())
            return modules_[it->second];

        cdb_module_state module;
        module.name = name;
        module.file = name + ".c";
        module_indexes_[name] = modules_.size();
        modules_.push_back(std::move(module));
        return modules_.back();
    }

    debug_function& ensure_function(cdb_module_state& module, const std::string& name) {
        const auto it = module.function_indexes.find(name);
        if (it != module.function_indexes.end())
            return info_.functions[it->second];

        debug_function fn;
        fn.name = name;
        fn.start = k_unset_address;
        fn.end = k_unset_address;
        info_.functions.push_back(std::move(fn));
        const auto index = info_.functions.size() - 1;
        module.function_indexes[name] = index;
        return info_.functions[index];
    }

    void parse_line(std::string_view line) {
        if (line.size() < 2 || line[1] != ':')
            return;

        const auto content = line.substr(2);
        switch (line[0]) {
        case 'M': parse_module(content);    break;
        case 'F': parse_function(content);  break;
        case 'S': parse_symbol(content);    break;
        case 'T': parse_type(content);      break;
        case 'L': parse_line_info(content); break;
        default:                            break;
        }
    }

    void parse_module(std::string_view content) {
        current_module_ = std::string(content);
        ensure_module(current_module_);
    }

    void parse_function(std::string_view content) {
        if (current_module_.empty())
            return;

        const auto first = content.find('$');
        if (first == std::string_view::npos)
            return;
        const auto second = content.find('$', first + 1);
        if (second == std::string_view::npos)
            return;

        auto& module = ensure_module(current_module_);
        auto name = strip_scope_qualifier(content.substr(first + 1, second - first - 1));
        auto& fn = ensure_function(module, name);

        if (const auto close = content.find("),"); close != std::string_view::npos) {
            auto fields = split(content.substr(close + 2), ',');
            if (auto cc = parse_abi_field(fields); cc.has_value())
                fn.convention = *cc;
        }
    }

    void parse_symbol(std::string_view content) {
        if (current_module_.empty() || content.empty())
            return;

        const auto first = content.find('$');
        if (first == std::string_view::npos)
            return;
        const auto second = content.find('$', first + 1);
        if (second == std::string_view::npos)
            return;

        auto& module = ensure_module(current_module_);
        const char scope_char = content.front();
        const auto scope_prefix = content.substr(1, first - 1);

        std::string name(content.substr(first + 1, second - first - 1));
        std::string type_name;
        var_storage storage = var_storage::unknown;
        int offset = 0;
        std::string reg_list;

        const auto open = content.find('(');
        if (open != std::string_view::npos) {
            const auto close = content.find(')', open);
            if (close != std::string_view::npos) {
                type_name = std::string(content.substr(open + 1, close - open - 1));
                if (close + 1 < content.size()) {
                    const auto suffix = content.substr(close + 1);
                    if (const auto bracket = suffix.find('[');
                        bracket != std::string_view::npos) {
                        const auto end = suffix.find(']', bracket);
                        if (end != std::string_view::npos)
                            reg_list = std::string(suffix.substr(bracket + 1, end - bracket - 1));
                    }

                    const auto parts = split(suffix, ',');
                    if (parts.size() >= 2 && !parts[1].empty())
                        storage = storage_from_code(parts[1].front());
                    if (parts.size() >= 4 && !parts[3].empty()) {
                        try {
                            offset = std::stoi(std::string(parts[3]));
                        } catch (...) {}
                    }
                }
            }
        }

        if (scope_char == 'L') {
            const auto dot = scope_prefix.find('.');
            if (dot == std::string_view::npos)
                return;

            auto parent = std::string(scope_prefix.substr(dot + 1));
            ensure_function(module, parent);

            debug_variable variable;
            variable.name = std::move(name);
            variable.parent = std::move(parent);
            variable.storage = storage;
            variable.offset = offset;
            variable.reg = std::move(reg_list);
            variable.type_name = std::move(type_name);
            info_.variables.push_back(std::move(variable));
            return;
        }

        debug_symbol symbol;
        symbol.name = name;
        symbol.address = static_cast<uint32_t>(offset);
        info_.symbols.push_back(symbol);

        debug_variable variable;
        variable.name = std::move(name);
        variable.storage = storage;
        variable.offset = offset;
        variable.reg = std::move(reg_list);
        variable.type_name = std::move(type_name);
        info_.variables.push_back(std::move(variable));
    }

    void parse_type(std::string_view) {}

    void parse_function_marker(std::string_view content, bool is_end) {
        const auto colon = content.rfind(':');
        if (colon == std::string_view::npos)
            return;

        uint32_t address = 0;
        try {
            address = static_cast<uint32_t>(
                std::stoul(std::string(content.substr(colon + 1)), nullptr, 16));
        } catch (...) {
            return;
        }

        const auto first = content.find('$');
        if (first == std::string_view::npos)
            return;
        const auto second = content.find('$', first + 1);
        if (second == std::string_view::npos)
            return;

        const auto module_name = std::string(content.substr(0, first));
        const auto function_name = strip_scope_qualifier(
            content.substr(first + 1, second - first - 1));

        auto assign = [&](cdb_module_state& module) {
            auto& function = ensure_function(module, function_name);
            if (!is_end)
                function.start = address;
            else
                function.end = address;
        };

        if (!module_name.empty()) {
            assign(ensure_module(module_name));
            return;
        }

        if (!current_module_.empty()) {
            assign(ensure_module(current_module_));
            return;
        }

        for (auto& module : modules_) {
            const auto it = module.function_indexes.find(function_name);
            if (it == module.function_indexes.end())
                continue;
            auto& function = info_.functions[it->second];
            if (!is_end)
                function.start = address;
            else
                function.end = address;
            return;
        }
    }

    void parse_line_info(std::string_view content) {
        if (content.empty())
            return;

        if (content[0] == 'F') {
            parse_function_marker(content.substr(1), false);
            return;
        }
        if (content.size() >= 2 && content[0] == 'X' && content[1] == 'F') {
            parse_function_marker(content.substr(2), true);
            return;
        }
        if (content[0] == 'G') {
            parse_function_marker(content.substr(1), false);
            return;
        }
        if (content.size() >= 2 && content[0] == 'X' && content[1] == 'G') {
            parse_function_marker(content.substr(2), true);
            return;
        }

        if (content[0] == 'A') {
            const auto first = content.find('$');
            if (first == std::string_view::npos)
                return;
            const auto second = content.find('$', first + 1);
            if (second == std::string_view::npos)
                return;
            const auto colon = content.rfind(':');
            if (colon == std::string_view::npos || colon <= second)
                return;

            const auto module_name = std::string(content.substr(first + 1, second - first - 1));
            int line_number = 0;
            try {
                line_number = std::stoi(std::string(content.substr(second + 1, colon - second - 1)));
            } catch (...) {
                return;
            }

            uint32_t address = 0;
            try {
                address = static_cast<uint32_t>(
                    std::stoul(std::string(content.substr(colon + 1)), nullptr, 16));
            } catch (...) {
                return;
            }

            ensure_module(module_name).asm_lines.emplace_back(line_number, address);
            return;
        }

        if (content[0] != 'C')
            return;

        const auto parts = split(content, '$');
        if (parts.size() < 4)
            return;

        std::string file(parts[1]);
        std::replace(file.begin(), file.end(), '\\', '/');

        uint32_t line_number = 0;
        try {
            line_number = static_cast<uint32_t>(std::stoul(std::string(parts[2]), nullptr, 10));
        } catch (...) {
            return;
        }

        uint32_t address = 0;
        if (const auto colon = parts.back().find(':'); colon != std::string_view::npos) {
            try {
                address = static_cast<uint32_t>(
                    std::stoul(std::string(parts.back().substr(colon + 1)), nullptr, 16));
            } catch (...) {}
        }

        auto& module = ensure_module(std::filesystem::path(file).stem().string());
        if (module.file == module.name + ".c")
            module.file = file;

        module.source_lines.push_back({address, line_number, intern_file(file)});
    }

    void finalize() {
        for (auto& module : modules_) {
            for (const auto& [_, index] : module.function_indexes) {
                auto& function = info_.functions[index];
                if (function.start == k_unset_address)
                    continue;

                for (const auto& line : module.source_lines) {
                    if (line.address >= function.start
                        && line.address < function.end) {
                        function.file_id = line.file_id;
                        function.line = line.line;
                        break;
                    }
                }
            }

            info_.lines.insert(
                info_.lines.end(),
                module.source_lines.begin(),
                module.source_lines.end());

            if (!module.asm_lines.empty()) {
                const auto file_id = intern_file(module.name + ".s");
                for (const auto& [line, address] : module.asm_lines)
                    info_.lines.push_back({address, static_cast<uint32_t>(line), file_id});
            }
        }
    }
};

} // namespace

// =========================================================================
// xbfd::cdb — streaming write mode constructor
// =========================================================================

cdb::cdb(std::ostream& asm_out, const std::string& src, const std::string& adb_path)
    : out_(&asm_out)
    , src_(src)
    , mod_(std::filesystem::path(src).stem().string())
    , adb_(adb_path)
{}

// =========================================================================
// xbfd::cdb — di_reader: read a .cdb file
// =========================================================================

std::optional<debug_info> cdb::read(const std::string& path) {
    return cdb_parser{}.parse(path);
}

// =========================================================================
// xbfd::cdb — di_writer: streaming CDB emission
// =========================================================================

void cdb::write_adb() const {
    if (adb_.empty()) return;
    std::ofstream f(adb_); if (!f) return;
    f << "M:" << mod_ << "\n";
    for (const auto& fn : fns_) {
        f << "F:G$" << fn.name << "$0_0$0({2}DF," << cdb_base(fn.ret) << "),C,0,0,0,0,0";
        if (fn.cc != calling_convention::unknown)
            f << ",ABI=" << to_string(fn.cc);
        f << "\n";
        for (const auto& lv : fn.locals) {
            f << "S:L" << mod_ << "." << fn.name << "$" << lv.name << "$1_0$" << fn.blk
              << "(" << cdb_type(lv.type) << ")," << sc(lv.sc_) << ",0," << lv.off;
            if (lv.sc_ == var_storage::reg && !lv.reg.empty()) f << ",[" << lv.reg << "]";
            f << "\n";
        }
    }
    for (const auto& g : gbls_)  f << "S:G$" << g.name << "$0_0$0(" << cdb_type(g.type) << "),E,0,0\n";
    for (const auto& s : fsyms_) f << "S:G$" << s.name << "$0_0$0({2}DF," << cdb_base(s.ret) << "),C,0,0\n";
}

void cdb::on_module_end(const std::string&) { write_adb(); }

void cdb::on_global(const std::string& n, const type_ref& t, bool s) {
    if (out_ && !s) *out_ << "G$" << n << "$0_0$0:\n";
    gbls_.push_back({n, t, s});
}

void cdb::on_function_begin(const std::string& c, const std::string&, bool g,
                            const type_ref& r, calling_convention cc) {
    if (out_) *out_ << "G$" << c << "$0$0:\n\t.globl\tG$" << c << "$0$0\n";
    cur_ = &fns_.emplace_back();
    cur_->name = c; cur_->global = g; cur_->ret = r; cur_->cc = cc; cur_->blk = blk_++;
    line_ = -1;
    fsyms_.push_back({c, r, g, cc});
}

void cdb::on_local(const std::string& n, const type_ref& t, var_storage s, int o, const std::string& r) {
    if (cur_) cur_->locals.push_back({n, t, s, o, r});
}

void cdb::on_function_end(const std::string&) { cur_ = nullptr; line_ = -1; }

void cdb::on_source_line(int l) {
    if (!out_ || l <= 0 || l == line_ || !cur_) return;
    line_ = l;
    const std::string s = "C$" + source_label_name(src_)
                        + "$" + std::to_string(l)
                        + "$1_0$" + std::to_string(cur_->blk);
    *out_ << s << ":\n\t.globl\t" << s << "\n";
}

} // namespace xbfd
