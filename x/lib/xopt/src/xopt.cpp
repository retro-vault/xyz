//
// xopt.cpp -- public optimizer interface implementation.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//

#include "xopt/xopt.h"
#include "xopt/z80peep.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <set>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace xopt {

namespace {

struct outline_candidate {
    std::string key;
    std::string section_key;
    asm_line section_line;
    size_t length = 0;
    long body_bytes = 0;
    long potential_savings = 0;
    std::vector<size_t> starts;
};

struct outlined_sequence {
    std::string label;
    std::string section_key;
    asm_line section_line;
    size_t length = 0;
    std::vector<size_t> starts;
    std::vector<asm_line> body;
};

struct merged_tail {
    std::string label;
    size_t length = 0;
    size_t canonical_start = 0;
    std::vector<size_t> starts;
};

std::string lowercase_ascii(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return text;
}

bool operand_has_token(const std::string &operand, const std::string &token) {
    const std::string text = lowercase_ascii(operand);
    auto token_char = [](unsigned char c) {
        return std::isalnum(c) || c == '_' || c == '\'';
    };

    size_t pos = 0;
    while ((pos = text.find(token, pos)) != std::string::npos) {
        const bool left_ok = pos == 0 || !token_char(text[pos - 1]);
        const size_t end = pos + token.size();
        const bool right_ok = end == text.size() || !token_char(text[end]);
        if (left_ok && right_ok)
            return true;
        pos = end;
    }
    return false;
}

bool uses_current_location(const std::string &operand) {
    if (operand.find('$') != std::string::npos)
        return true;

    // GNU symbols such as .Ltmp are stable when moved.  A standalone dot,
    // however, denotes the current assembly location and is not outlinable.
    for (size_t i = 0; i < operand.size(); ++i) {
        if (operand[i] != '.')
            continue;
        if (i + 1 == operand.size() ||
            (!std::isalpha(static_cast<unsigned char>(operand[i + 1])) &&
             operand[i + 1] != '_')) {
            return true;
        }
    }
    return false;
}

bool executable_section_directive(const asm_line &line) {
    const std::string mnemonic = lowercase_ascii(line.mnemonic);
    const std::string operands = lowercase_ascii(line.operands);
    if (mnemonic == ".text")
        return true;
    if (mnemonic == ".area")
        return operands.find("code") != std::string::npos ||
               operands.find("home") != std::string::npos;
    if (mnemonic == ".section")
        return operands.find(".text") != std::string::npos ||
               operands.find("code") != std::string::npos ||
               operands.find("\"ax\"") != std::string::npos;
    return false;
}

bool any_section_directive(const asm_line &line) {
    const std::string mnemonic = lowercase_ascii(line.mnemonic);
    return mnemonic == ".area" || mnemonic == ".text" ||
           mnemonic == ".data" || mnemonic == ".bss" ||
           mnemonic == ".rodata" || mnemonic == ".section" ||
           mnemonic == ".tdata";
}

void collect_executable_sections(
        const std::vector<asm_line> &lines,
        std::vector<std::string> &sections,
        std::unordered_map<std::string, asm_line> &section_lines) {
    sections.assign(lines.size(), {});
    std::string current_section;
    for (size_t i = 0; i < lines.size(); ++i) {
        if (any_section_directive(lines[i])) {
            if (executable_section_directive(lines[i])) {
                current_section = lines[i].to_string();
                section_lines[current_section] = lines[i];
            } else {
                current_section.clear();
            }
        }
        sections[i] = current_section;
    }
}

bool outlinable_instruction(const asm_line &line) {
    if (!line.label.empty() || line.is_comment || line.mnemonic.empty() ||
        !line.comment.empty() || line.mnemonic[0] == '.') {
        return false;
    }

    static const std::unordered_set<std::string> excluded = {
        "call", "ret", "reti", "retn", "rst", "jp", "jr", "djnz",
        "push", "pop", "exx", "di", "ei", "halt", "im", "nop",
        "in", "out", "ini", "inir", "ind", "indr",
        "outi", "otir", "outd", "otdr"
    };
    const std::string mnemonic = lowercase_ascii(line.mnemonic);
    if (excluded.count(mnemonic) != 0)
        return false;

    // CALL temporarily changes SP and advances R, so sequences that observe
    // either machine register cannot be moved behind a helper call.
    if (operand_has_token(line.operands, "sp") ||
        operand_has_token(line.operands, "r") ||
        uses_current_location(line.operands)) {
        return false;
    }
    return true;
}

bool tail_mergeable_instruction(const asm_line &line) {
    if (!line.label.empty() || line.is_comment || line.mnemonic.empty() ||
        (!line.comment.empty() && line.comment.rfind("xopt-ix:", 0) != 0) ||
        line.mnemonic[0] == '.') {
        return false;
    }

    // A jump to a shared tail preserves the complete machine state, including
    // SP, so stack operations are legal here even though they are forbidden
    // in call-based outlining.  Only instructions that can leave the
    // straight-line tail (other than an ordinary returning CALL) are rejected.
    static const std::unordered_set<std::string> control = {
        "jp", "jr", "djnz", "ret", "reti", "retn", "rst"
    };
    if (control.count(lowercase_ascii(line.mnemonic)) != 0)
        return false;
    return !uses_current_location(line.operands);
}

long outlined_savings(long body_bytes, size_t occurrences) {
    constexpr long kCallBytes = 3;
    constexpr long kRetBytes = 1;
    if (occurrences < 2)
        return 0;
    return static_cast<long>(occurrences) * body_bytes -
           (static_cast<long>(occurrences) * kCallBytes + body_bytes + kRetBytes);
}

std::vector<int> instruction_ix_offsets(const asm_line &line) {
    std::vector<int> offsets;
    const std::string &operands = line.operands;
    size_t pos = 0;
    while ((pos = operands.find("(ix)", pos)) != std::string::npos) {
        size_t begin = pos;
        while (begin > 0) {
            const unsigned char c = static_cast<unsigned char>(operands[begin - 1]);
            if (!std::isdigit(c) && c != '+' && c != '-' && !std::isspace(c))
                break;
            --begin;
        }
        try {
            offsets.push_back(std::stoi(operands.substr(begin, pos - begin)));
        } catch (...) {
            // Non-numeric IX expressions remain a full barrier below.
            offsets.clear();
            offsets.push_back(0x10000);
            return offsets;
        }
        pos += 4;
    }
    return offsets;
}

std::string outline_ix_metadata(const std::vector<asm_line> &body) {
    std::vector<int> offsets;
    for (const asm_line &line : body) {
        std::vector<int> line_offsets = instruction_ix_offsets(line);
        offsets.insert(offsets.end(), line_offsets.begin(), line_offsets.end());
    }
    if (std::find(offsets.begin(), offsets.end(), 0x10000) != offsets.end())
        return "xopt-ix:any";
    std::sort(offsets.begin(), offsets.end());
    offsets.erase(std::unique(offsets.begin(), offsets.end()), offsets.end());

    if (offsets.empty())
        return "xopt-ix:none";

    std::string metadata = "xopt-ix:";
    for (size_t i = 0; i < offsets.size(); ++i) {
        if (i != 0)
            metadata.push_back(',');
        metadata += std::to_string(offsets[i]);
    }
    return metadata;
}

std::vector<int> ix_frame_depth_before(const std::vector<asm_line> &lines) {
    constexpr int kUnknownDepth = -1;
    std::vector<int> depths(lines.size(), kUnknownDepth);
    int depth = kUnknownDepth;
    bool hl_immediate_known = false;
    int hl_immediate = 0;
    bool hl_sp_relative_known = false;
    int hl_sp_relative = 0;

    auto compact = [](std::string text) {
        text = lowercase_ascii(std::move(text));
        text.erase(std::remove_if(text.begin(), text.end(),
                                  [](unsigned char c) { return std::isspace(c); }),
                   text.end());
        return text;
    };
    auto parse_immediate = [](std::string text, int &value) {
        text.erase(std::remove_if(text.begin(), text.end(),
                                  [](unsigned char c) { return std::isspace(c); }),
                   text.end());
        if (!text.empty() && text.front() == '#')
            text.erase(text.begin());
        if (text.empty())
            return false;
        char *end = nullptr;
        const long parsed = std::strtol(text.c_str(), &end, 0);
        if (end == text.c_str() || *end != '\0')
            return false;
        value = static_cast<int>(parsed);
        return true;
    };

    for (size_t i = 0; i < lines.size(); ++i) {
        depths[i] = depth;
        const asm_line &line = lines[i];
        const std::string mnemonic = lowercase_ascii(line.mnemonic);
        const std::string operands = compact(line.operands);

        if (mnemonic == "call" && operands == "__sdcc_enter_ix") {
            // The helper establishes IX at the unallocated frame boundary.
            depth = 0;
            hl_immediate_known = false;
            hl_sp_relative_known = false;
            continue;
        }
        if (depth == kUnknownDepth)
            continue;

        if (mnemonic == "push") {
            depth += 2;
        } else if (mnemonic == "pop") {
            if (depth < 2)
                depth = kUnknownDepth;
            else
                depth -= 2;
        } else if (mnemonic == "dec" && operands == "sp") {
            ++depth;
        } else if (mnemonic == "inc" && operands == "sp") {
            if (depth < 1)
                depth = kUnknownDepth;
            else
                --depth;
        }

        const size_t comma = operands.find(',');
        const std::string lhs = comma == std::string::npos
            ? operands : operands.substr(0, comma);
        const std::string rhs = comma == std::string::npos
            ? std::string{} : operands.substr(comma + 1);

        if (mnemonic == "ld" && lhs == "hl") {
            int immediate = 0;
            hl_immediate_known = parse_immediate(rhs, immediate);
            if (hl_immediate_known)
                hl_immediate = immediate;
            hl_sp_relative_known = false;
        } else if (mnemonic == "add" && operands == "hl,sp") {
            hl_sp_relative_known = hl_immediate_known;
            if (hl_sp_relative_known)
                hl_sp_relative = hl_immediate;
            hl_immediate_known = false;
        } else if (mnemonic == "ld" && lhs == "sp" && rhs == "hl") {
            if (!hl_sp_relative_known) {
                depth = kUnknownDepth;
            } else {
                depth -= hl_sp_relative;
                if (depth < 0)
                    depth = kUnknownDepth;
            }
            hl_sp_relative_known = false;
            hl_immediate_known = false;
        } else if (mnemonic == "ld" && lhs == "sp" && rhs == "ix") {
            depth = 0;
            hl_sp_relative_known = false;
            hl_immediate_known = false;
        } else if (mnemonic == "call" || mnemonic == "ret" ||
                   mnemonic == "reti" || mnemonic == "retn" ||
                   (mnemonic == "jp" && operands == "__sdcc_leave_ix")) {
            hl_sp_relative_known = false;
            hl_immediate_known = false;
            if (mnemonic != "call")
                depth = kUnknownDepth;
        } else if (mnemonic == "pop" && operands == "hl") {
            hl_sp_relative_known = false;
            hl_immediate_known = false;
        } else if ((mnemonic == "inc" && operands == "hl") ||
                   (mnemonic == "dec" && operands == "hl") ||
                   (mnemonic == "add" && lhs == "hl") ||
                   (mnemonic == "adc" && lhs == "hl") ||
                   (mnemonic == "sbc" && lhs == "hl") ||
                   (mnemonic == "ld" && (lhs == "h" || lhs == "l")) ||
                   (mnemonic == "ex" &&
                    (operands == "de,hl" || operands == "hl,de" ||
                     operands == "(sp),hl" || operands == "hl,(sp)"))) {
            hl_sp_relative_known = false;
            hl_immediate_known = false;
        }
    }
    return depths;
}

bool outline_occurrence_stack_safe(const std::vector<asm_line> &lines,
                                   size_t start, size_t length,
                                   int allocated_depth) {
    int required_depth = 0;
    for (size_t i = start; i < start + length; ++i) {
        for (int offset : instruction_ix_offsets(lines[i])) {
            if (offset == 0x10000)
                return false;
            if (offset < 0)
                required_depth = std::max(required_depth, -offset);
        }
    }
    return required_depth == 0 || allocated_depth >= required_depth;
}

std::string compact_unused_temp_frames(const std::string &asm_text,
                                       bool size_bias) {
    std::vector<asm_line> lines;
    std::istringstream input(asm_text);
    std::string raw;
    while (std::getline(input, raw))
        lines.push_back(asm_line::parse(raw));

    auto parse_field = [](const std::string &comment, const char *key,
                          int &value, size_t *digit_pos = nullptr,
                          size_t *digit_len = nullptr) {
        const std::string lower = lowercase_ascii(comment);
        size_t pos = lower.find(key);
        if (pos == std::string::npos)
            return false;
        pos += std::strlen(key);
        if (pos >= lower.size() ||
            !std::isdigit(static_cast<unsigned char>(lower[pos]))) {
            return false;
        }
        const size_t begin = pos;
        value = 0;
        while (pos < lower.size() &&
               std::isdigit(static_cast<unsigned char>(lower[pos]))) {
            value = value * 10 + (lower[pos] - '0');
            ++pos;
        }
        if (digit_pos)
            *digit_pos = begin;
        if (digit_len)
            *digit_len = pos - begin;
        return true;
    };

    auto normalized_operands = [](std::string text) {
        text = lowercase_ascii(std::move(text));
        text.erase(std::remove_if(text.begin(), text.end(),
                                  [](unsigned char c) {
                                      return std::isspace(c) != 0;
                                  }),
                   text.end());
        return text;
    };

    auto parse_immediate = [&](const std::string &operand, int &value) {
        std::string text = normalized_operands(operand);
        if (!text.empty() && text.front() == '#')
            text.erase(text.begin());
        if (text.empty())
            return false;
        char *end = nullptr;
        const long parsed = std::strtol(text.c_str(), &end, 0);
        if (!end || *end != '\0')
            return false;
        value = static_cast<int>(parsed);
        return true;
    };

    auto is_known_frame_ix_use = [&](const asm_line &line) {
        const std::string ops = normalized_operands(line.operands);
        return ((line.mnemonic == "push" || line.mnemonic == "pop") &&
                ops == "ix") ||
               (line.mnemonic == "ld" &&
                (ops == "ix,#0" || ops == "ix,0" || ops == "sp,ix")) ||
               (line.mnemonic == "add" && ops == "ix,sp");
    };

    auto allocation_bytes = [&](size_t begin, size_t end,
                                size_t &alloc_end) {
        int bytes = 0;
        size_t pos = begin;
        while (pos < end && lines[pos].label.empty()) {
            const std::string ops = normalized_operands(lines[pos].operands);
            if (lines[pos].mnemonic == "push" && ops == "af") {
                bytes += 2;
            } else if (lines[pos].mnemonic == "dec" && ops == "sp") {
                ++bytes;
            } else {
                break;
            }
            ++pos;
        }
        alloc_end = pos;
        return bytes;
    };

    bool changed = false;
    for (size_t prologue = 0; prologue < lines.size(); ++prologue) {
        const std::string lower_comment = lowercase_ascii(lines[prologue].comment);
        if (lower_comment.find("prologue:") == std::string::npos)
            continue;

        int locals = 0;
        int temp_frame = 0;
        size_t temp_digits = 0;
        size_t temp_digit_count = 0;
        if (!parse_field(lines[prologue].comment, "locals=", locals) ||
            !parse_field(lines[prologue].comment, "temp_frame=", temp_frame,
                         &temp_digits, &temp_digit_count) ||
            temp_frame <= 0) {
            continue;
        }

        size_t fn_end = lines.size();
        for (size_t i = prologue + 1; i < lines.size(); ++i) {
            if (any_section_directive(lines[i])) {
                fn_end = i;
                break;
            }
        }

        const int old_total = locals + temp_frame;
        std::set<int> used_temp_offsets;
        bool opaque_frame_use = false;
        for (size_t i = prologue + 1; i < fn_end; ++i) {
            const std::vector<int> offsets = instruction_ix_offsets(lines[i]);
            for (int offset : offsets) {
                if (offset == 0x10000) {
                    opaque_frame_use = true;
                    break;
                }
                if (offset < -old_total) {
                    opaque_frame_use = true;
                    break;
                }
                if (offset < -locals)
                    used_temp_offsets.insert(offset);
            }
            if (opaque_frame_use)
                break;

            const std::string ops = lowercase_ascii(lines[i].operands);
            if (operand_has_token(ops, "ix") && offsets.empty() &&
                !is_known_frame_ix_use(lines[i])) {
                opaque_frame_use = true;
                break;
            }
            if (operand_has_token(ops, "sp")) {
                const std::string normalized = normalized_operands(ops);
                const bool known_sp_use =
                    (lines[i].mnemonic == "add" && normalized == "ix,sp") ||
                    (lines[i].mnemonic == "ld" && normalized == "sp,ix") ||
                    (lines[i].mnemonic == "dec" && normalized == "sp");
                if (!known_sp_use) {
                    opaque_frame_use = true;
                    break;
                }
            }
        }
        if (opaque_frame_use)
            continue;

        std::unordered_map<int, int> remapped_offsets;
        int next_depth = locals + 1;
        for (auto it = used_temp_offsets.rbegin();
             it != used_temp_offsets.rend(); ++it) {
            remapped_offsets.emplace(*it, -next_depth++);
        }
        const int new_total = locals +
                              static_cast<int>(used_temp_offsets.size());
        if (new_total >= old_total)
            continue;

        size_t alloc_begin = lines.size();
        size_t alloc_end = lines.size();
        const size_t search_end = std::min(fn_end, prologue + 32);
        for (size_t i = prologue + 1; i < search_end; ++i) {
            if (!lines[i].label.empty())
                break;
            size_t run_end = i;
            if (allocation_bytes(i, search_end, run_end) == old_total) {
                alloc_begin = i;
                alloc_end = run_end;
                break;
            }

            if (i + 2 >= search_end || lines[i].mnemonic != "ld" ||
                lines[i + 1].mnemonic != "add" ||
                lines[i + 2].mnemonic != "ld") {
                continue;
            }
            const std::string first = normalized_operands(lines[i].operands);
            const std::string second = normalized_operands(lines[i + 1].operands);
            const std::string third = normalized_operands(lines[i + 2].operands);
            const size_t comma = first.find(',');
            int amount = 0;
            if (comma != std::string::npos && first.substr(0, comma) == "hl" &&
                parse_immediate(first.substr(comma + 1), amount) &&
                amount == -old_total && second == "hl,sp" &&
                third == "sp,hl") {
                alloc_begin = i;
                alloc_end = i + 3;
                break;
            }
        }
        if (alloc_begin == lines.size())
            continue;

        auto remap_numeric_ix_operands = [&](asm_line &line) {
            size_t pos = 0;
            while ((pos = line.operands.find("(ix)", pos)) !=
                   std::string::npos) {
                size_t begin = pos;
                while (begin > 0) {
                    const unsigned char c = static_cast<unsigned char>(
                        line.operands[begin - 1]);
                    if (!std::isdigit(c) && c != '+' && c != '-' &&
                        !std::isspace(c)) {
                        break;
                    }
                    --begin;
                }
                int old_offset = 0;
                try {
                    old_offset = std::stoi(
                        line.operands.substr(begin, pos - begin));
                } catch (...) {
                    return false;
                }
                auto mapped = remapped_offsets.find(old_offset);
                if (mapped == remapped_offsets.end() ||
                    mapped->second == old_offset) {
                    pos += 4;
                    continue;
                }
                const std::string replacement =
                    std::to_string(mapped->second);
                line.operands.replace(begin, pos - begin, replacement);
                pos = begin + replacement.size() + 4;
            }
            return true;
        };

        bool remap_ok = true;
        for (size_t i = prologue + 1; i < fn_end; ++i) {
            if (!remap_numeric_ix_operands(lines[i])) {
                remap_ok = false;
                break;
            }
        }
        if (!remap_ok)
            continue;

        std::vector<asm_line> replacement;
        // PUSH/DEC minimizes size through eight bytes.  For speed it wins
        // only through four bytes (22 cycles versus 27 for LD/ADD/LD);
        // a five-byte PUSH/PUSH/DEC sequence already costs 28 cycles.
        const int push_threshold = size_bias ? 8 : 4;
        if (new_total <= push_threshold) {
            for (int i = 0; i < new_total / 2; ++i)
                replacement.push_back(asm_line::parse("\tpush\taf"));
            if (new_total & 1)
                replacement.push_back(asm_line::parse("\tdec\tsp"));
        } else {
            replacement.push_back(asm_line::parse(
                "\tld\thl, #" + std::to_string(-new_total)));
            replacement.push_back(asm_line::parse("\tadd\thl, sp"));
            replacement.push_back(asm_line::parse("\tld\tsp, hl"));
        }

        lines.erase(lines.begin() + static_cast<std::ptrdiff_t>(alloc_begin),
                    lines.begin() + static_cast<std::ptrdiff_t>(alloc_end));
        lines.insert(lines.begin() + static_cast<std::ptrdiff_t>(alloc_begin),
                     replacement.begin(), replacement.end());
        lines[prologue].comment.replace(
            temp_digits, temp_digit_count,
            std::to_string(new_total - locals));
        changed = true;
    }

    if (!changed)
        return asm_text;

    std::string result;
    for (const asm_line &line : lines)
        result += line.to_string();
    return result;
}

// Late optimization can leave a canonical temporary allocation entirely dead
// after backend frame-omission has already made its decision.  Remove the
// allocation and IX save/setup/restore only when the emitted body proves it
// independent of IX, SP, and the allocation's HL result: no indexed
// references, explicit stack traffic, alternate return, or second epilogue is
// accepted.  The allocation must exactly agree with compiler frame metadata.
std::string remove_unused_ix_frames(const std::string &asm_text) {
    std::vector<asm_line> lines;
    std::istringstream input(asm_text);
    std::string raw;
    while (std::getline(input, raw))
        lines.push_back(asm_line::parse(raw));

    auto normalized_operands = [](std::string text) {
        text = lowercase_ascii(std::move(text));
        text.erase(std::remove_if(text.begin(), text.end(),
                                  [](unsigned char c) {
                                      return std::isspace(c) != 0;
                                  }),
                   text.end());
        return text;
    };

    auto parse_nonnegative_field = [](const std::string &comment,
                                      const char *key,
                                      int &value) {
        const std::string lower = lowercase_ascii(comment);
        size_t pos = lower.find(key);
        if (pos == std::string::npos)
            return false;
        pos += std::strlen(key);
        if (pos >= lower.size() ||
            !std::isdigit(static_cast<unsigned char>(lower[pos]))) {
            return false;
        }
        size_t end = pos;
        while (end < lower.size() &&
               std::isdigit(static_cast<unsigned char>(lower[end]))) {
            ++end;
        }
        try {
            value = std::stoi(lower.substr(pos, end - pos));
        } catch (...) {
            return false;
        }
        return true;
    };

    auto matches = [&](size_t index, const char *mnemonic,
                       const char *operands) {
        return index < lines.size() &&
               lines[index].label.empty() &&
               lowercase_ascii(lines[index].mnemonic) == mnemonic &&
               normalized_operands(lines[index].operands) == operands;
    };

    auto matches_frame_allocation = [&](size_t index, int frame_bytes) {
        if (index >= lines.size() || !lines[index].label.empty() ||
            lowercase_ascii(lines[index].mnemonic) != "ld") {
            return false;
        }
        const std::string operands =
            normalized_operands(lines[index].operands);
        constexpr const char *prefix = "hl,";
        if (operands.rfind(prefix, 0) != 0)
            return false;
        std::string immediate = operands.substr(std::strlen(prefix));
        if (!immediate.empty() && immediate.front() == '#')
            immediate.erase(immediate.begin());
        if (immediate.empty())
            return false;
        char *end = nullptr;
        const long value = std::strtol(immediate.c_str(), &end, 0);
        return end != immediate.c_str() && *end == '\0' &&
               value == -frame_bytes;
    };

    auto allocation_flags_dead_before_observation =
        [&](size_t start, size_t end) {
            for (size_t i = start; i < end; ++i) {
                const std::string mnemonic =
                    lowercase_ascii(lines[i].mnemonic);
                if (mnemonic.empty())
                    continue;

                // The canonical `add hl,sp` allocation changes C/H/N only.
                // These instructions overwrite that complete affected set
                // without consuming it.
                if (mnemonic == "sub" || mnemonic == "and" ||
                    mnemonic == "or" || mnemonic == "xor" ||
                    mnemonic == "cp" || mnemonic == "scf" ||
                    mnemonic == "sla" || mnemonic == "sra" ||
                    mnemonic == "srl" || mnemonic == "sll" ||
                    mnemonic == "rlc" || mnemonic == "rrc" ||
                    mnemonic == "rlca" || mnemonic == "rrca") {
                    return true;
                }
                if (mnemonic == "add") {
                    const std::string operands =
                        normalized_operands(lines[i].operands);
                    if (operands == "a" ||
                        operands.rfind("a,", 0) == 0) {
                        return true;
                    }
                }

                // Stack-allocation flags are ABI-dead at an ordinary return,
                // but may not cross arbitrary control flow, calls, DAA, or an
                // instruction that consumes carry.
                if (mnemonic == "ret")
                    return true;
                if (mnemonic == "call" || mnemonic == "rst" ||
                    mnemonic == "jp" || mnemonic == "jr" ||
                    mnemonic == "djnz" || mnemonic == "reti" ||
                    mnemonic == "retn" || mnemonic == "daa" ||
                    mnemonic == "adc" || mnemonic == "sbc" ||
                    mnemonic == "rl" || mnemonic == "rr" ||
                    mnemonic == "rla" || mnemonic == "rra" ||
                    mnemonic == "ccf") {
                    return false;
                }
            }
            return false;
        };

    std::vector<bool> erase(lines.size(), false);
    bool changed = false;
    for (size_t comment = 0; comment < lines.size(); ++comment) {
        const std::string lower_comment =
            lowercase_ascii(lines[comment].comment);
        int locals = -1;
        int temp_frame = -1;
        if (lower_comment.find("prologue:") == std::string::npos ||
            !parse_nonnegative_field(lines[comment].comment, "locals=",
                                     locals) ||
            !parse_nonnegative_field(lines[comment].comment, "temp_frame=",
                                     temp_frame) ||
            locals != 0) {
            continue;
        }

        const size_t push_ix = comment + 1;
        const size_t load_ix = comment + 2;
        const size_t add_sp = comment + 3;
        if (!matches(push_ix, "push", "ix") ||
            !(matches(load_ix, "ld", "ix,#0") ||
              matches(load_ix, "ld", "ix,0")) ||
            !matches(add_sp, "add", "ix,sp")) {
            continue;
        }

        size_t body_start = add_sp + 1;
        size_t alloc_load = lines.size();
        size_t alloc_add = lines.size();
        size_t alloc_store = lines.size();
        if (temp_frame > 0) {
            alloc_load = body_start;
            alloc_add = body_start + 1;
            alloc_store = body_start + 2;
            if (!matches_frame_allocation(alloc_load, temp_frame) ||
                !matches(alloc_add, "add", "hl,sp") ||
                !matches(alloc_store, "ld", "sp,hl")) {
                continue;
            }
            body_start += 3;
        }

        size_t fn_end = lines.size();
        for (size_t i = body_start; i < lines.size(); ++i) {
            if (any_section_directive(lines[i])) {
                fn_end = i;
                break;
            }
        }

        size_t restore_sp = lines.size();
        size_t pop_ix = lines.size();
        size_t return_index = lines.size();
        bool safe = true;
        if (temp_frame > 0 &&
            !allocation_flags_dead_before_observation(body_start, fn_end)) {
            safe = false;
        }
        for (size_t i = body_start; i < fn_end; ++i) {
            if (i + 2 < fn_end && matches(i, "ld", "sp,ix") &&
                matches(i + 1, "pop", "ix") &&
                matches(i + 2, "ret", "")) {
                if (restore_sp != lines.size()) {
                    safe = false;
                    break;
                }
                restore_sp = i;
                pop_ix = i + 1;
                return_index = i + 2;
                i += 2;
                continue;
            }

            const std::string mnemonic = lowercase_ascii(lines[i].mnemonic);
            if (operand_has_token(lines[i].operands, "ix") ||
                operand_has_token(lines[i].operands, "sp") ||
                (temp_frame > 0 &&
                 (operand_has_token(lines[i].operands, "hl") ||
                  operand_has_token(lines[i].operands, "h") ||
                  operand_has_token(lines[i].operands, "l"))) ||
                mnemonic == "push" || mnemonic == "pop" ||
                mnemonic == "ret" || mnemonic == "reti" ||
                mnemonic == "retn") {
                safe = false;
                break;
            }
        }
        if (!safe || restore_sp == lines.size() ||
            return_index == lines.size()) {
            continue;
        }

        erase[push_ix] = true;
        erase[load_ix] = true;
        erase[add_sp] = true;
        if (temp_frame > 0) {
            erase[alloc_load] = true;
            erase[alloc_add] = true;
            erase[alloc_store] = true;
        }
        erase[restore_sp] = true;
        erase[pop_ix] = true;
        if (temp_frame > 0) {
            std::string lower = lowercase_ascii(lines[comment].comment);
            size_t value_begin = lower.find("temp_frame=") +
                                 std::strlen("temp_frame=");
            size_t value_end = value_begin;
            while (value_end < lines[comment].comment.size() &&
                   std::isdigit(static_cast<unsigned char>(
                       lines[comment].comment[value_end]))) {
                ++value_end;
            }
            lines[comment].comment.replace(value_begin,
                                           value_end - value_begin, "0");
        }
        changed = true;
    }

    if (!changed)
        return asm_text;

    std::string result;
    for (size_t i = 0; i < lines.size(); ++i)
        if (!erase[i])
            result += lines[i].to_string();
    return result;
}

bool unconditional_tail_instruction(const asm_line &line) {
    if (!line.label.empty() || line.is_comment || line.mnemonic.empty() ||
        !line.comment.empty()) {
        return false;
    }
    const std::string mnemonic = lowercase_ascii(line.mnemonic);
    if ((mnemonic == "ret" || mnemonic == "reti" || mnemonic == "retn") &&
        line.operands.empty()) {
        return true;
    }
    return (mnemonic == "jp" || mnemonic == "jr") &&
           line.operands.find(',') == std::string::npos;
}

long merged_tail_savings(long body_bytes, size_t occurrences) {
    constexpr long kJumpBytes = 3;
    if (occurrences < 2 || body_bytes <= kJumpBytes)
        return 0;
    return static_cast<long>(occurrences - 1) * (body_bytes - kJumpBytes);
}

bool is_private_codegen_label(const std::string &label) {
    if (label.rfind("__xcc_", 0) == 0)
        return true;

    static const std::vector<std::string> control_prefixes = {
        "__postinc_", "__postdec_", "__shiftxor", "__cmp_",
        "__lcmp_", "__llcmp_", "__fcmp_"
    };
    for (const std::string &prefix : control_prefixes)
        if (label.rfind(prefix, 0) == 0)
            return true;

    constexpr const char *end_suffix = "_end";
    return label.rfind("__", 0) == 0 &&
           label.size() > std::strlen(end_suffix) &&
           label.compare(label.size() - std::strlen(end_suffix),
                         std::strlen(end_suffix), end_suffix) == 0;
}

void collect_operand_symbols(const std::string &operands,
                             std::unordered_set<std::string> &symbols) {
    auto symbol_char = [](unsigned char c) {
        return std::isalnum(c) || c == '_' || c == '.' || c == '$' || c == '\'';
    };

    size_t begin = 0;
    while (begin < operands.size()) {
        while (begin < operands.size() &&
               !symbol_char(static_cast<unsigned char>(operands[begin]))) {
            ++begin;
        }
        size_t end = begin;
        while (end < operands.size() &&
               symbol_char(static_cast<unsigned char>(operands[end]))) {
            ++end;
        }
        if (end > begin)
            symbols.insert(operands.substr(begin, end - begin));
        begin = end;
    }
}

std::string remove_unreferenced_internal_labels(const std::string &asm_text) {
    std::vector<asm_line> lines;
    std::istringstream input(asm_text);
    std::string raw;
    while (std::getline(input, raw))
        lines.push_back(asm_line::parse(raw));

    std::unordered_map<std::string, size_t> definitions;
    std::unordered_set<std::string> references;
    for (const asm_line &line : lines) {
        if (is_private_codegen_label(line.label))
            ++definitions[line.label];
        collect_operand_symbols(line.operands, references);
    }

    bool changed = false;
    for (asm_line &line : lines) {
        if (!is_private_codegen_label(line.label) ||
            definitions[line.label] != 1 || references.count(line.label) != 0) {
            continue;
        }
        line.label.clear();
        line.is_label = false;
        line.is_global_label = false;
        changed = true;
    }
    if (!changed)
        return asm_text;

    std::string output;
    for (const asm_line &line : lines)
        output += line.to_string();
    return output;
}

std::string merge_repeated_tails(const std::string &asm_text) {
    std::vector<asm_line> lines;
    std::istringstream input(asm_text);
    std::string raw;
    while (std::getline(input, raw))
        lines.push_back(asm_line::parse(raw));

    constexpr size_t kMinInstructions = 2;
    constexpr size_t kMaxInstructions = 32;
    if (lines.size() < kMinInstructions * 2)
        return asm_text;

    std::vector<std::string> sections;
    std::unordered_map<std::string, asm_line> section_lines;
    collect_executable_sections(lines, sections, section_lines);
    std::unordered_map<std::string, outline_candidate> candidate_map;
    for (size_t end = 0; end < lines.size(); ++end) {
        if (sections[end].empty() || !unconditional_tail_instruction(lines[end]))
            continue;

        std::string body_key = lines[end].mnemonic + '\t' + lines[end].operands;
        body_key.push_back('\0');
        std::string body_text = lines[end].to_string();
        for (size_t length = 2;
             length <= kMaxInstructions && length <= end + 1;
             ++length) {
            const size_t start = end + 1 - length;
            if (sections[start] != sections[end] ||
                !tail_mergeable_instruction(lines[start])) {
                break;
            }

            body_key = lines[start].mnemonic + '\t' + lines[start].operands +
                       std::string(1, '\0') + body_key;
            body_text = lines[start].to_string() + body_text;
            const std::string key = sections[end] + '\x1f' + body_key;
            auto [it, inserted] = candidate_map.emplace(key, outline_candidate{});
            outline_candidate &candidate = it->second;
            if (inserted) {
                candidate.key = key;
                candidate.section_key = sections[end];
                candidate.section_line = section_lines[sections[end]];
                candidate.length = length;
                const assembly_cost cost = estimate_z80_assembly_cost(body_text);
                candidate.body_bytes = cost.unknown_instructions == 0
                    ? cost.bytes
                    : -1;
            }
            candidate.starts.push_back(start);
        }
    }

    std::vector<outline_candidate> candidates;
    candidates.reserve(candidate_map.size());
    for (auto &entry : candidate_map) {
        outline_candidate candidate = std::move(entry.second);
        if (candidate.body_bytes <= 0 || candidate.starts.size() < 2)
            continue;
        candidate.potential_savings = merged_tail_savings(
            candidate.body_bytes, candidate.starts.size());
        if (candidate.potential_savings > 0)
            candidates.push_back(std::move(candidate));
    }

    std::sort(candidates.begin(), candidates.end(),
              [](const outline_candidate &lhs, const outline_candidate &rhs) {
                  if (lhs.potential_savings != rhs.potential_savings)
                      return lhs.potential_savings > rhs.potential_savings;
                  if (lhs.body_bytes != rhs.body_bytes)
                      return lhs.body_bytes > rhs.body_bytes;
                  if (lhs.length != rhs.length)
                      return lhs.length > rhs.length;
                  return lhs.key < rhs.key;
              });

    std::unordered_set<std::string> labels;
    for (const asm_line &line : lines) {
        if (!line.label.empty())
            labels.insert(line.label);
    }

    std::vector<bool> claimed(lines.size(), false);
    std::vector<merged_tail> selected;
    size_t next_tail_id = 0;
    for (const outline_candidate &candidate : candidates) {
        std::vector<size_t> starts;
        for (size_t start : candidate.starts) {
            bool available = true;
            for (size_t i = start; i < start + candidate.length; ++i) {
                if (claimed[i]) {
                    available = false;
                    break;
                }
            }
            if (available)
                starts.push_back(start);
        }
        if (merged_tail_savings(candidate.body_bytes, starts.size()) <= 0)
            continue;

        std::string label;
        do {
            label = "__xopt_tail_" + std::to_string(next_tail_id++);
        } while (labels.count(label) != 0);
        labels.insert(label);

        merged_tail tail;
        tail.label = label;
        tail.length = candidate.length;
        tail.canonical_start = starts.front();
        tail.starts = std::move(starts);
        for (size_t start : tail.starts) {
            for (size_t i = start; i < start + tail.length; ++i)
                claimed[i] = true;
        }
        selected.push_back(std::move(tail));
    }

    if (selected.empty())
        return asm_text;

    struct tail_replacement {
        size_t length = 0;
        std::string label;
    };
    std::unordered_map<size_t, std::string> inserted_labels;
    std::unordered_map<size_t, tail_replacement> replacements;
    for (const merged_tail &tail : selected) {
        inserted_labels.emplace(tail.canonical_start, tail.label);
        for (size_t i = 1; i < tail.starts.size(); ++i) {
            replacements.emplace(
                tail.starts[i], tail_replacement{tail.length, tail.label});
        }
    }

    std::string output;
    for (size_t i = 0; i < lines.size();) {
        auto label_it = inserted_labels.find(i);
        if (label_it != inserted_labels.end())
            output += asm_line::parse(label_it->second + ":").to_string();

        auto replacement_it = replacements.find(i);
        if (replacement_it == replacements.end()) {
            output += lines[i].to_string();
            ++i;
            continue;
        }
        output += asm_line::parse("\tjp\t" + replacement_it->second.label).to_string();
        i += replacement_it->second.length;
    }
    return output;
}

std::string outline_repeated_sequences(const std::string &asm_text) {
    std::vector<asm_line> lines;
    std::istringstream input(asm_text);
    std::string raw;
    while (std::getline(input, raw))
        lines.push_back(asm_line::parse(raw));

    constexpr size_t kMinInstructions = 3;
    constexpr size_t kMaxInstructions = 32;
    if (lines.size() < kMinInstructions * 2)
        return asm_text;

    std::vector<std::string> sections;
    std::unordered_map<std::string, asm_line> section_lines;
    collect_executable_sections(lines, sections, section_lines);
    const std::vector<int> frame_depths = ix_frame_depth_before(lines);

    std::unordered_map<std::string, outline_candidate> candidate_map;
    for (size_t start = 0; start < lines.size(); ++start) {
        if (sections[start].empty())
            continue;

        std::string body_key;
        std::string body_text;
        for (size_t length = 1;
             length <= kMaxInstructions && start + length <= lines.size();
             ++length) {
            const size_t index = start + length - 1;
            if (sections[index] != sections[start] ||
                !outlinable_instruction(lines[index])) {
                break;
            }

            body_key += lines[index].mnemonic;
            body_key.push_back('\t');
            body_key += lines[index].operands;
            body_key.push_back('\0');
            body_text += lines[index].to_string();
            if (length < kMinInstructions)
                continue;

            const std::string key = sections[start] + '\x1f' + body_key;
            auto [it, inserted] = candidate_map.emplace(key, outline_candidate{});
            outline_candidate &candidate = it->second;
            if (inserted) {
                candidate.key = key;
                candidate.section_key = sections[start];
                candidate.section_line = section_lines[sections[start]];
                candidate.length = length;
                const assembly_cost cost = estimate_z80_assembly_cost(body_text);
                if (cost.unknown_instructions != 0) {
                    candidate.body_bytes = -1;
                } else {
                    candidate.body_bytes = cost.bytes;
                }
            }
            candidate.starts.push_back(start);
        }
    }

    std::vector<outline_candidate> candidates;
    candidates.reserve(candidate_map.size());
    for (auto &entry : candidate_map) {
        outline_candidate candidate = std::move(entry.second);
        if (candidate.body_bytes <= 0 || candidate.starts.size() < 2)
            continue;

        size_t count = 0;
        size_t next_start = 0;
        for (size_t start : candidate.starts) {
            if (count == 0 || start >= next_start) {
                ++count;
                next_start = start + candidate.length;
            }
        }
        candidate.potential_savings = outlined_savings(candidate.body_bytes, count);
        if (candidate.potential_savings > 0)
            candidates.push_back(std::move(candidate));
    }

    std::sort(candidates.begin(), candidates.end(),
              [](const outline_candidate &lhs, const outline_candidate &rhs) {
                  if (lhs.potential_savings != rhs.potential_savings)
                      return lhs.potential_savings > rhs.potential_savings;
                  if (lhs.body_bytes != rhs.body_bytes)
                      return lhs.body_bytes > rhs.body_bytes;
                  if (lhs.length != rhs.length)
                      return lhs.length > rhs.length;
                  return lhs.key < rhs.key;
              });

    std::unordered_set<std::string> labels;
    for (const asm_line &line : lines) {
        if (!line.label.empty())
            labels.insert(line.label);
    }

    std::vector<bool> claimed(lines.size(), false);
    std::vector<outlined_sequence> selected;
    size_t next_outline_id = 0;
    for (const outline_candidate &candidate : candidates) {
        std::vector<size_t> starts;
        size_t next_start = 0;
        for (size_t start : candidate.starts) {
            if (!starts.empty() && start < next_start)
                continue;
            if (!outline_occurrence_stack_safe(
                    lines, start, candidate.length, frame_depths[start])) {
                continue;
            }
            bool available = true;
            for (size_t i = start; i < start + candidate.length; ++i) {
                if (claimed[i]) {
                    available = false;
                    break;
                }
            }
            if (!available)
                continue;
            starts.push_back(start);
            next_start = start + candidate.length;
        }

        if (outlined_savings(candidate.body_bytes, starts.size()) <= 0)
            continue;

        std::string label;
        do {
            label = "__xopt_outline_" + std::to_string(next_outline_id++);
        } while (labels.count(label) != 0);
        labels.insert(label);

        outlined_sequence sequence;
        sequence.label = label;
        sequence.section_key = candidate.section_key;
        sequence.section_line = candidate.section_line;
        sequence.length = candidate.length;
        sequence.starts = std::move(starts);
        sequence.body.assign(
            lines.begin() + static_cast<std::ptrdiff_t>(sequence.starts.front()),
            lines.begin() + static_cast<std::ptrdiff_t>(sequence.starts.front() +
                                                        sequence.length));
        for (size_t start : sequence.starts) {
            for (size_t i = start; i < start + sequence.length; ++i)
                claimed[i] = true;
        }
        selected.push_back(std::move(sequence));
    }

    if (selected.empty())
        return asm_text;

    struct replacement {
        size_t length = 0;
        std::string label;
    };
    std::unordered_map<size_t, replacement> replacements;
    std::unordered_map<std::string, std::string> ix_metadata;
    for (const outlined_sequence &sequence : selected) {
        ix_metadata.emplace(sequence.label, outline_ix_metadata(sequence.body));
        for (size_t start : sequence.starts)
            replacements.emplace(start, replacement{sequence.length, sequence.label});
    }

    std::string output;
    for (size_t i = 0; i < lines.size();) {
        auto replacement_it = replacements.find(i);
        if (replacement_it == replacements.end()) {
            output += lines[i].to_string();
            ++i;
            continue;
        }
        asm_line call = asm_line::parse("\tcall\t" + replacement_it->second.label);
        call.comment = ix_metadata[replacement_it->second.label];
        output += call.to_string();
        i += replacement_it->second.length;
    }

    std::string emitted_section;
    for (const outlined_sequence &sequence : selected) {
        if (sequence.section_key != emitted_section) {
            output += "\n";
            output += sequence.section_line.to_string();
            emitted_section = sequence.section_key;
        }
        output += asm_line::parse(sequence.label + ":").to_string();
        for (const asm_line &line : sequence.body)
            output += line.to_string();
        output += asm_line::parse("\tret").to_string();
    }
    return output;
}

int choose_pass_budget(const std::string &asm_text,
                       optimization_level level) {
    const size_t bytes = asm_text.size();

    if (bytes > 350000) {
        if (level == optimization_level::o1)
            return 0;
        if (level == optimization_level::o2 ||
            level == optimization_level::os ||
            level == optimization_level::of ||
            level == optimization_level::o3)
            return 2;
        return 3;
    }

    if (bytes > 200000) {
        if (level == optimization_level::o1)
            return 2;
        if (level == optimization_level::o2 ||
            level == optimization_level::os ||
            level == optimization_level::of ||
            level == optimization_level::o3)
            return 4;
        return 6;
    }

    return 10;
}

} // namespace

bool uses_speed_biased_rules(optimization_level level) {
    return level == optimization_level::of ||
           level == optimization_level::o3;
}

std::string optimize_z80_assembly(const std::string &asm_text,
                                  optimization_level level) {
    if (level == optimization_level::none)
        return asm_text;
    const int pass_budget = choose_pass_budget(asm_text, level);
    const bool size_bias = level == optimization_level::os;
    const size_t asm_line_count =
        static_cast<size_t>(std::count(asm_text.begin(), asm_text.end(), '\n'));
    // The old 8K-line cutoff skipped every ordinary peephole rule for large
    // generated functions, even though the bounded pass budget above already
    // keeps their compile time under control.  This left substantial generic
    // cleanup on the table in modules just beyond that threshold.  Retain a
    // guard for pathological translation-limit inputs, but cover normal large
    // applications before outlining changes their shape.
    constexpr size_t kMaxIterativePeepholeLines = 16000;
    const bool scalable_size_only =
        size_bias && asm_line_count > kMaxIterativePeepholeLines;
    std::string optimized = scalable_size_only
        ? asm_text
        : z80_peep::optimize(
              asm_text, uses_speed_biased_rules(level), pass_budget, size_bias);
    if (size_bias || uses_speed_biased_rules(level))
        optimized = compact_unused_temp_frames(optimized, size_bias);
    if (uses_speed_biased_rules(level))
        optimized = remove_unused_ix_frames(optimized);
    if (level == optimization_level::os) {
        // Tail merging can expose an outline, and that outline can in turn
        // expose another common tail.  Four bounded rounds reach a fixed
        // point for large generated modules that still changed after the old
        // two-round cap, without making this an unbounded compile-time loop.
        for (int round = 0; round < 4; ++round) {
            const std::string before = optimized;
            optimized = remove_unreferenced_internal_labels(optimized);
            optimized = merge_repeated_tails(optimized);
            optimized = outline_repeated_sequences(optimized);
            // Outlined calls may consume any register used by their body.
            // Ordinary call-clobber liveness can delete those live-in values,
            // so only the control-flow layout cleanup below may follow.
            if (optimized == before)
                break;
        }
        // The last outlining round can make local labels unreferenced.  Drop
        // them before the restricted final peephole so they do not conceal
        // newly adjacent control-flow and addressing patterns.
        optimized = remove_unreferenced_internal_labels(optimized);
        optimized = z80_peep::optimize_outlined_layout(optimized);
    }
    return optimized;
}

std::string optimize_assembly(const std::string &asm_text,
                              const optimizer_options &options) {
    return optimize_z80_assembly(asm_text, options.level);
}

} // namespace xopt
