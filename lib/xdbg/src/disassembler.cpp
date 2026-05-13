/*
 * Implements the xdbg disassembly adapters and syntax formatters
 * used to decode Z80 instructions and render them in native or
 * SDCC-oriented textual forms.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <memory>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include <z80ex_dasm.h>

#include <xdbg/disassembler.hpp>
#include <xdbg/error.hpp>

namespace xdbg {

    namespace {

        struct z80ex_read_context {
            const memory_reader* memory = nullptr;
        };

        static Z80EX_BYTE z80ex_readbyte_cb(Z80EX_WORD addr, void* user_data) {
            const auto* context = static_cast<const z80ex_read_context*>(user_data);
            return context->memory->read8(addr);
        }

        std::string trim(const std::string& value) {
            const auto start = value.find_first_not_of(" \t\r\n");
            if (start == std::string::npos) {
                return "";
            }
            const auto end = value.find_last_not_of(" \t\r\n");
            return value.substr(start, end - start + 1);
        }

        std::vector<std::string> split_operands(const std::string& text) {
            std::vector<std::string> operands;
            std::size_t start = 0;
            int depth = 0;

            for (std::size_t i = 0; i < text.size(); ++i) {
                if (text[i] == '(') {
                    ++depth;
                } else if (text[i] == ')') {
                    --depth;
                } else if (text[i] == ',' && depth == 0) {
                    operands.push_back(trim(text.substr(start, i - start)));
                    start = i + 1;
                }
            }

            if (start < text.size()) {
                operands.push_back(trim(text.substr(start)));
            }
            return operands;
        }

        std::string to_lower(std::string value) {
            std::transform(value.begin(), value.end(), value.begin(),
                [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
            return value;
        }

        std::string format_hex_value(uint32_t value) {
            std::ostringstream out;
            out << "0x" << std::hex << std::nouppercase << value << std::dec;
            return out.str();
        }

        bool is_hex_string(const std::string& value) {
            return !value.empty() && std::all_of(value.begin(), value.end(),
                [](unsigned char ch) { return std::isxdigit(ch) != 0; });
        }

        std::string normalize_indexed_operand(const std::string& operand) {
            static const std::regex pattern(
                R"(\((IX|IY)\+#([0-9A-Fa-f]+)\))");
            std::smatch match;
            if (!std::regex_match(operand, match, pattern)) {
                return "";
            }

            const auto reg = to_lower(match[1].str());
            const uint32_t raw = static_cast<uint32_t>(
                std::stoul(match[2].str(), nullptr, 16));
            const int8_t disp = static_cast<int8_t>(raw & 0xFF);

            std::ostringstream out;
            out << static_cast<int>(disp) << "(" << reg << ")";
            return out.str();
        }

        std::string normalize_z80_operand(const std::string& operand) {
            if (const auto indexed = normalize_indexed_operand(operand); !indexed.empty()) {
                return indexed;
            }

            if (operand.size() > 1 && operand[0] == '#' && is_hex_string(operand.substr(1))) {
                return "#" + format_hex_value(
                    static_cast<uint32_t>(std::stoul(operand.substr(1), nullptr, 16)));
            }

            static const std::regex direct_pattern(R"(\(#([0-9A-Fa-f]+)\))");
            std::smatch match;
            if (std::regex_match(operand, match, direct_pattern)) {
                return "(#" + format_hex_value(
                    static_cast<uint32_t>(std::stoul(match[1].str(), nullptr, 16))) + ")";
            }

            return to_lower(operand);
        }

        class native_formatter final : public syntax_formatter {
        public:
            std::string format(const disassembled_instruction& instruction) const override {
                return instruction.text;
            }
        };

        class sdcc_z80_formatter final : public syntax_formatter {
        public:
            std::string format(const disassembled_instruction& instruction) const override {
                std::ostringstream out;
                out << to_lower(instruction.mnemonic);
                if (!instruction.operands.empty()) {
                    out << "\t";
                    for (std::size_t i = 0; i < instruction.operands.size(); ++i) {
                        if (i != 0) {
                            out << ", ";
                        }
                        out << normalize_z80_operand(instruction.operands[i]);
                    }
                }
                return out.str();
            }
        };

        class z80ex_disassembler final : public disassembler {
        public:
            cpu_kind cpu() const override {
                return cpu_kind::z80;
            }

            disassembled_instruction disassemble_one(
                uint32_t address, const memory_reader& memory) const override
            {
                z80ex_read_context context{&memory};
                char buffer[128];
                int t_states = 0;
                int t_states_alt = 0;
                const int length = z80ex_dasm(
                    buffer, sizeof(buffer), 0, &t_states, &t_states_alt,
                    z80ex_readbyte_cb, static_cast<Z80EX_WORD>(address), &context);

                if (length <= 0) {
                    throw error("failed to disassemble instruction");
                }

                disassembled_instruction instruction;
                instruction.cpu = cpu_kind::z80;
                instruction.address = address;
                instruction.t_states = t_states;
                instruction.t_states_alt = t_states_alt;

                for (int i = 0; i < length; ++i) {
                    instruction.bytes.push_back(memory.read8(address + static_cast<uint32_t>(i)));
                }

                instruction.text = trim(buffer);
                const auto split_at = instruction.text.find_first_of(" \t");
                if (split_at == std::string::npos) {
                    instruction.mnemonic = instruction.text;
                } else {
                    instruction.mnemonic = trim(instruction.text.substr(0, split_at));
                    instruction.operands = split_operands(
                        instruction.text.substr(split_at + 1));
                }

                return instruction;
            }
        };

    } // namespace

    std::unique_ptr<disassembler> make_z80_disassembler() {
        return std::make_unique<z80ex_disassembler>();
    }

    std::unique_ptr<syntax_formatter> make_native_formatter() {
        return std::make_unique<native_formatter>();
    }

    std::unique_ptr<syntax_formatter> make_sdcc_z80_formatter() {
        return std::make_unique<sdcc_z80_formatter>();
    }

} // namespace xdbg
