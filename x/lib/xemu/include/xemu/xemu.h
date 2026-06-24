// xemu.h — host-side Z80 emulator library.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <memory>
#include <string>
#include <span>
#include <vector>

#include <rsp/rsp.h>
#include <xz80/cpu_state.h>

namespace xemu {

struct register_image {
    uint16_t af = 0;
    uint16_t bc = 0;
    uint16_t de = 0;
    uint16_t hl = 0;
    uint16_t ix = 0;
    uint16_t iy = 0;
    uint16_t sp = 0;
    uint16_t pc = 0;
    uint8_t  i  = 0;
    uint8_t  r  = 0;
};

enum class stop_reason {
    none,
    breakpoint,
    stepped,
    halted,
    step_limit,
    fault
};

struct stop_result {
    stop_reason reason = stop_reason::none;
    std::size_t steps = 0;
    uint16_t pc = 0;
    std::string message;
};

class machine {
public:
    explicit machine(uint8_t fill = 0x00);
    ~machine();

    machine(const machine&) = delete;
    machine& operator=(const machine&) = delete;
    machine(machine&&) noexcept;
    machine& operator=(machine&&) noexcept;

    void reset() noexcept;
    void clear_memory(uint8_t fill = 0x00) noexcept;

    void load_binary(const std::filesystem::path& path, uint16_t origin);
    void load_bytes(uint16_t origin, std::span<const uint8_t> bytes) noexcept;

    std::vector<uint8_t> read_memory(uint32_t address, std::size_t length) const;
    uint8_t read_byte(uint16_t address) const noexcept;
    void write_memory(uint32_t address, std::span<const uint8_t> bytes) noexcept;
    void write_byte(uint16_t address, uint8_t value) noexcept;

    xz80::cpu_state snapshot() const noexcept;
    void restore(const xz80::cpu_state& state) noexcept;

    register_image registers() const noexcept;
    void set_registers(const register_image& regs) noexcept;
    void set_pc(uint16_t pc) noexcept;
    void set_sp(uint16_t sp) noexcept;

    void bind_stdin(uint16_t port, std::istream& input) noexcept;
    void bind_stdin_status_data(
        uint16_t status_port,
        uint16_t data_port,
        std::istream& input) noexcept;
    void bind_stdout(uint16_t port, std::ostream& output) noexcept;
    void clear_stdin() noexcept;
    void clear_stdout() noexcept;
    void clear_io() noexcept;

    void insert_breakpoint(uint16_t address);
    void remove_breakpoint(uint16_t address);
    void clear_breakpoints() noexcept;

    stop_result continue_execution(std::size_t max_steps = 1'000'000) noexcept;
    stop_result step_instruction() noexcept;

    bool halted() const noexcept;

private:
    struct impl;
    std::unique_ptr<impl> impl_;
};

class rsp_target_adapter final : public rsp::target {
public:
    explicit rsp_target_adapter(machine& emu) noexcept;

    std::vector<uint8_t> read_registers() override;
    void write_registers(const std::vector<uint8_t>& data) override;
    std::vector<uint8_t> read_memory(uint32_t address, std::size_t length) override;
    void write_memory(uint32_t address, const std::vector<uint8_t>& data) override;
    std::string cont() override;
    std::string step() override;
    std::string stop_reason() override;
    void insert_breakpoint(uint32_t address) override;
    void remove_breakpoint(uint32_t address) override;
    void detach() override;

private:
    machine* emu_;
    std::string last_stop_ = "S05";
};

class remote_session {
public:
    void connect(const std::string& host, uint16_t port);
    void close();
    bool is_connected() const;

    rsp::stop_reply query_stop();

    register_image read_registers();
    void write_registers(const register_image& regs);
    void set_pc(uint16_t pc);
    void set_sp(uint16_t sp);

    std::vector<uint8_t> read_memory(uint32_t address, std::size_t length);
    void write_memory(uint32_t address, std::span<const uint8_t> data);
    void load_binary(const std::filesystem::path& path, uint16_t origin);

    rsp::stop_reply continue_execution();
    rsp::stop_reply step_instruction();
    void pause();

    void insert_breakpoint(uint32_t address);
    void remove_breakpoint(uint32_t address);
    void detach();

private:
    rsp::client client_;
};

} // namespace xemu
