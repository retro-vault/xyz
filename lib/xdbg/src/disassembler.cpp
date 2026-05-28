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

#include <xz80/disassembler.hpp>
#include <xz80/memory.hpp>

#include <xdbg/disassembler.hpp>
#include <xdbg/error.hpp>

namespace xdbg {

    namespace {

        // Adapts xdbg::memory_reader (uint32_t addresses) to xz80::IMemory
        // (uint16_t addresses) so the xz80 disassembler can fetch bytes via
        // the xdbg memory abstraction without any allocation.
        class memory_bridge final : public xz80::IMemory {
        public:
            explicit memory_bridge(const memory_reader& r) noexcept : reader_(r) {}
            uint8_t read(uint16_t addr) const noexcept override {
                return reader_.read8(addr);
            }
            void write(uint16_t, uint8_t) noexcept override {}
        private:
            const memory_reader& reader_;
        };

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

        // xz80 disassembler produces (IX+N) / (IX-N) with signed decimal displacement.
        // Convert to SDCC form: N(ix).
        std::string normalize_indexed_operand(const std::string& operand) {
            static const std::regex pattern(R"(\((IX|IY)([+-]\d+)\))");
            std::smatch match;
            if (!std::regex_match(operand, match, pattern)) {
                return "";
            }
            const auto reg = to_lower(match[1].str());
            const int disp = std::stoi(match[2].str());
            std::ostringstream out;
            out << disp << "(" << reg << ")";
            return out.str();
        }

        std::string normalize_z80_operand(const std::string& operand) {
            if (const auto indexed = normalize_indexed_operand(operand); !indexed.empty()) {
                return indexed;
            }

            // #NNNN or #NN — keep the # prefix, normalize hex case
            if (operand.size() > 1 && operand[0] == '#' && is_hex_string(operand.substr(1))) {
                return "#" + format_hex_value(
                    static_cast<uint32_t>(std::stoul(operand.substr(1), nullptr, 16)));
            }

            // (#NNNN) — direct address
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

        class xz80_disassembler final : public disassembler {
        public:
            cpu_kind cpu() const override {
                return cpu_kind::z80;
            }

            disassembled_instruction disassemble_one(
                uint32_t address, const memory_reader& memory) const override
            {
                const memory_bridge bridge{memory};
                const xz80::disasm_result r =
                    dasm_.disassemble(static_cast<uint16_t>(address), bridge);

                disassembled_instruction instr;
                instr.cpu          = cpu_kind::z80;
                instr.address      = address;
                instr.t_states     = r.t_states;
                instr.t_states_alt = r.t_states2;
                instr.text         = r.text;

                for (uint8_t i = 0; i < r.length; ++i) {
                    instr.bytes.push_back(r.bytes[i]);
                }

                const auto split_at = instr.text.find_first_of(" \t");
                if (split_at == std::string::npos) {
                    instr.mnemonic = instr.text;
                } else {
                    instr.mnemonic = trim(instr.text.substr(0, split_at));
                    instr.operands = split_operands(instr.text.substr(split_at + 1));
                }

                return instr;
            }

        private:
            xz80::disassembler dasm_;
        };

    } // namespace

    std::unique_ptr<disassembler> make_z80_disassembler() {
        return std::make_unique<xz80_disassembler>();
    }

    std::unique_ptr<syntax_formatter> make_native_formatter() {
        return std::make_unique<native_formatter>();
    }

    std::unique_ptr<syntax_formatter> make_sdcc_z80_formatter() {
        return std::make_unique<sdcc_z80_formatter>();
    }

} // namespace xdbg
