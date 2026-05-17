/*
 * Declares the xdbg disassembly interfaces, memory adapters, and
 * formatter factories used to decode and render target code.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */

#ifndef XDBG_DISASSEMBLER_HPP
#define XDBG_DISASSEMBLER_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace xdbg {

    // Processor family supported by a disassembler instance.
    enum class cpu_kind {
        unknown,   // Unknown or unspecified CPU family.
        z80        // Zilog Z80 family disassembly.
    };

    // Result of decoding a single instruction from memory.
    struct disassembled_instruction {
        cpu_kind cpu = cpu_kind::unknown;    // CPU family that produced this decode.
        uint32_t address = 0;                // Address of the decoded instruction.
        std::vector<uint8_t> bytes;          // Raw instruction bytes.
        std::string text;                    // Human-readable instruction text.
        std::string mnemonic;                // Decoded mnemonic token.
        std::vector<std::string> operands;   // Decoded operand list.
        int t_states = 0;                    // Primary timing in T-states.
        int t_states_alt = 0;                // Alternate timing in T-states, when applicable.
    };

    /*
     * Abstract byte-addressable memory source for disassembly.
     */
    class memory_reader {
    public:
        // Destroy the reader through the interface.
        //
        // Parameters:
        //      None.
        //
        // Returns:
        //      Nothing.
        virtual ~memory_reader() = default;

        /*
         * Read one byte from the specified address.
         *
         * Parameters:
         *      address     - Address to read from.
         *
         * Returns:
         *      Byte value at the requested address.
         */
        virtual uint8_t read8(uint32_t address) const = 0;
    };

    /*
     * Memory reader backed directly by a byte vector.
     */
    class vector_memory_reader : public memory_reader {
    public:
        /*
         * Bind the reader to an existing byte vector.
         *
         * Parameters:
         *      bytes       - Instruction or memory bytes to expose.
         */
        explicit vector_memory_reader(const std::vector<uint8_t>& bytes)
            : bytes_(bytes) {}

        /*
         * Read one byte from the backing vector.
         *
         * Parameters:
         *      address     - Zero-based byte index.
         *
         * Returns:
         *      Byte at the requested index, or `0x00` when out of range.
         */
        uint8_t read8(uint32_t address) const override {
            return address < bytes_.size() ? bytes_[address] : 0x00;
        }

    private:
        const std::vector<uint8_t>& bytes_;
    };

    /*
     * Abstract formatter for rendered disassembly text.
     */
    class syntax_formatter {
    public:
        // Destroy the formatter through the interface.
        //
        // Parameters:
        //      None.
        //
        // Returns:
        //      Nothing.
        virtual ~syntax_formatter() = default;

        /*
         * Render one decoded instruction into display text.
         *
         * Parameters:
         *      instruction - Instruction to format.
         *
         * Returns:
         *      Formatted instruction text.
         */
        virtual std::string format(const disassembled_instruction& instruction) const = 0;
    };

    /*
     * Abstract instruction decoder for a specific CPU family.
     */
    class disassembler {
    public:
        // Destroy the disassembler through the interface.
        //
        // Parameters:
        //      None.
        //
        // Returns:
        //      Nothing.
        virtual ~disassembler() = default;

        /*
         * Report the CPU family handled by this disassembler.
         *
         * Returns:
         *      Supported CPU kind.
         */
        virtual cpu_kind cpu() const = 0;

        /*
         * Decode one instruction from the supplied memory source.
         *
         * Parameters:
         *      address     - Start address of the instruction.
         *      memory      - Memory source used for byte fetches.
         *
         * Returns:
         *      Fully decoded instruction record.
         */
        virtual disassembled_instruction disassemble_one(
            uint32_t address, const memory_reader& memory) const = 0;
    };

    /*
     * Create a Z80 disassembler instance.
     *
     * Returns:
     *      Heap-allocated disassembler implementation.
     */
    std::unique_ptr<disassembler> make_z80_disassembler();

    /*
     * Create a formatter that preserves the disassembler's native syntax.
     *
     * Returns:
     *      Heap-allocated syntax formatter.
     */
    std::unique_ptr<syntax_formatter> make_native_formatter();

    /*
     * Create a formatter that emits SDCC-style Z80 syntax.
     *
     * Returns:
     *      Heap-allocated syntax formatter.
     */
    std::unique_ptr<syntax_formatter> make_sdcc_z80_formatter();

} // namespace xdbg

#endif // XDBG_DISASSEMBLER_HPP
