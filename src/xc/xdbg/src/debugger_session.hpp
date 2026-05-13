#ifndef XDBG_DEBUGGER_SESSION_HPP
#define XDBG_DEBUGGER_SESSION_HPP

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <xdbg/debugger_host.hpp>
#include <xdbg/xdbg.hpp>
#include <xdbgstub/xdbgstub.hpp>

class debugger_session : public debugger_host {
public:
    void set_exec_path(const std::filesystem::path& path) override;
    const std::optional<std::filesystem::path>& exec_path() const override;

    void load_symbols_file(const std::filesystem::path& path) override;
    void maybe_load_default_symbols() override;
    const std::optional<std::filesystem::path>& symbols_path() const override;
    bool has_symbols() const override;
    const xdbg::document* symbols() const override;

    void connect_remote(const std::string& target) override;
    bool is_connected() const override;

    breakpoint_entry add_breakpoint(const std::string& expression) override;
    void delete_breakpoint(int id) override;
    void delete_all_breakpoints() override;
    const std::vector<breakpoint_entry>& breakpoints() const override;

    xdbgstub::cpu_state read_registers() override;
    void write_registers(const xdbgstub::cpu_state& state) override;
    xdbgstub::target_status raw_status() override;
    stop_snapshot status() override;
    stop_snapshot continue_execution() override;
    stop_snapshot step_instruction() override;
    stop_snapshot pause_execution() override;
    void detach() override;

    uint32_t resolve_address_expression(const std::string& expression) const override;

    std::optional<source_location> current_source_location() override;
    std::optional<source_location> source_location_for_address(uint32_t address) const override;
    std::optional<source_location> source_location_for_function(
        const std::string& function_name) const override;
    line_info_result info_line_current() override;
    line_info_result info_line_argument(const std::string& argument) override;
    std::vector<source_line_view> list_source(
        const std::optional<std::string>& argument,
        uint32_t context_before = 5,
        uint32_t context_after = 5) override;

    std::vector<disassembly_line> disassemble(
        uint32_t address, std::size_t count) override;
    std::vector<uint8_t> read_memory(uint32_t address, std::size_t length) override;

    std::vector<const xdbg::variable*> visible_variables(uint32_t pc) const override;
    std::vector<std::pair<std::string, uint32_t>> register_values() override;

private:
    stop_snapshot make_stop_snapshot(const xdbgstub::target_status& status);
    std::optional<std::string> read_source_line(
        const std::filesystem::path& path, uint32_t target_line) const;
    std::vector<source_line_view> source_window(
        const std::filesystem::path& path,
        uint32_t target_line,
        uint32_t context_before,
        uint32_t context_after) const;

    const xdbg::source_file* find_file(uint32_t file_id) const;
    const xdbg::function* find_function_by_name(const std::string& name) const;
    const xdbg::symbol* find_symbol_by_name(const std::string& name) const;
    const xdbg::function* find_function_for_pc(uint32_t pc) const;
    const xdbg::line_entry* find_line_for_pc(uint32_t pc) const;
    const xdbg::line_entry* find_line_for_function(
        const xdbg::function& function) const;
    const xdbg::line_entry* find_line_by_file_and_line(
        uint32_t file_id, uint32_t line_number) const;
    std::optional<uint32_t> find_line_end_address(
        const xdbg::line_entry& line) const;

    std::optional<std::filesystem::path> exec_path_;
    std::optional<xdbg::document> symbols_;
    std::optional<std::filesystem::path> symbols_path_;
    xdbgstub::client remote_;
    std::vector<breakpoint_entry> breakpoints_;
    int next_breakpoint_id_ = 1;
};

#endif
