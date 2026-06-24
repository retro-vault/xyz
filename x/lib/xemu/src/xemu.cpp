// xemu.cpp — host-side Z80 emulator library implementation.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <istream>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <rsp/rsp.h>
#include <xemu/xemu.h>
#include <xz80/cpu.h>
#include <xz80/memory.h>
#include <xz80/ports.h>

namespace xemu {

namespace {

register_image image_from_state(const xz80::cpu_state& state) noexcept {
    register_image image;
    image.af = state.af;
    image.bc = state.bc;
    image.de = state.de;
    image.hl = state.hl;
    image.ix = state.ix;
    image.iy = state.iy;
    image.sp = state.sp;
    image.pc = state.pc;
    image.i  = state.i;
    image.r  = state.r;
    return image;
}

xz80::cpu_state merge_state(
    const xz80::cpu_state& current,
    const register_image& regs) noexcept
{
    xz80::cpu_state state = current;
    state.af = regs.af;
    state.bc = regs.bc;
    state.de = regs.de;
    state.hl = regs.hl;
    state.ix = regs.ix;
    state.iy = regs.iy;
    state.sp = regs.sp;
    state.pc = regs.pc;
    state.i  = regs.i;
    state.r  = regs.r;
    return state;
}

std::vector<uint8_t> read_file_bytes(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("cannot open binary: " + path.string());
    return std::vector<uint8_t>(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>());
}

std::string to_rsp_stop(const stop_result& stop) {
    switch (stop.reason) {
    case stop_reason::halted:
        return "W00";
    case stop_reason::step_limit:
        return "S02";
    case stop_reason::fault:
        return "S04";
    case stop_reason::breakpoint:
    case stop_reason::stepped:
    case stop_reason::none:
    default:
        return "S05";
    }
}

register_image unpack_registers(const std::vector<uint8_t>& data) {
    register_image regs;
    if (data.size() < 18) return regs;

    auto u16 = [&](std::size_t index) -> uint16_t {
        return static_cast<uint16_t>(data[index]) |
            static_cast<uint16_t>(static_cast<uint16_t>(data[index + 1]) << 8);
    };

    regs.af = u16(0);
    regs.bc = u16(2);
    regs.de = u16(4);
    regs.hl = u16(6);
    regs.ix = u16(8);
    regs.iy = u16(10);
    regs.sp = u16(12);
    regs.pc = u16(14);
    regs.i  = data[16];
    regs.r  = data[17];
    return regs;
}

std::vector<uint8_t> pack_registers(const register_image& regs) {
    std::vector<uint8_t> data(18, 0);
    auto put16 = [&](std::size_t index, uint16_t value) {
        data[index] = static_cast<uint8_t>(value & 0xFF);
        data[index + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
    };

    put16(0, regs.af);
    put16(2, regs.bc);
    put16(4, regs.de);
    put16(6, regs.hl);
    put16(8, regs.ix);
    put16(10, regs.iy);
    put16(12, regs.sp);
    put16(14, regs.pc);
    data[16] = regs.i;
    data[17] = regs.r;
    return data;
}

} // namespace

struct machine::impl {
    class port_mux final : public xz80::IPorts {
    public:
        uint8_t in(uint16_t port) noexcept override {
            if (stdin_stream_ != nullptr && stdin_status_port_.has_value() &&
                port == stdin_status_port_.value()) {
                const int ch = stdin_stream_->peek();
                if (ch == std::char_traits<char>::eof()) {
                    stdin_stream_->clear();
                    return 0x00;
                }
                return 0x01;
            }

            if (stdin_stream_ != nullptr && stdin_data_port_.has_value() &&
                port == stdin_data_port_.value()) {
                const int ch = stdin_stream_->get();
                if (ch == std::char_traits<char>::eof()) {
                    stdin_stream_->clear();
                    return 0xFF;
                }
                return static_cast<uint8_t>(ch);
            }
            return 0xFF;
        }

        void out(uint16_t port, uint8_t value) noexcept override {
            if (stdout_stream_ != nullptr && stdout_port_.has_value() &&
                port == stdout_port_.value()) {
                stdout_stream_->put(static_cast<char>(value));
                stdout_stream_->flush();
            }
        }

        void bind_stdin(uint16_t port, std::istream& input) noexcept {
            stdin_status_port_.reset();
            stdin_data_port_ = port;
            stdin_stream_ = &input;
        }

        void bind_stdin_status_data(
            uint16_t status_port,
            uint16_t data_port,
            std::istream& input) noexcept
        {
            stdin_status_port_ = status_port;
            stdin_data_port_ = data_port;
            stdin_stream_ = &input;
        }

        void bind_stdout(uint16_t port, std::ostream& output) noexcept {
            stdout_port_ = port;
            stdout_stream_ = &output;
        }

        void clear_stdin() noexcept {
            stdin_status_port_.reset();
            stdin_data_port_.reset();
            stdin_stream_ = nullptr;
        }

        void clear_stdout() noexcept {
            stdout_port_.reset();
            stdout_stream_ = nullptr;
        }

    private:
        std::optional<uint16_t> stdin_status_port_;
        std::optional<uint16_t> stdin_data_port_;
        std::optional<uint16_t> stdout_port_;
        std::istream* stdin_stream_ = nullptr;
        std::ostream* stdout_stream_ = nullptr;
    };

    explicit impl(uint8_t fill)
        : mem(fill)
        , cpu(mem, ports)
    {
        cpu.reset();
    }

    bool has_breakpoint(uint16_t address) const noexcept {
        return std::find(breakpoints.begin(), breakpoints.end(), address) !=
            breakpoints.end();
    }

    port_mux ports;
    xz80::flat_memory mem;
    xz80::cpu cpu;
    std::vector<uint16_t> breakpoints;
};

machine::machine(uint8_t fill)
    : impl_(std::make_unique<impl>(fill))
{}

machine::~machine() = default;
machine::machine(machine&&) noexcept = default;
machine& machine::operator=(machine&&) noexcept = default;

void machine::reset() noexcept {
    impl_->cpu.reset();
}

void machine::clear_memory(uint8_t fill) noexcept {
    for (uint32_t address = 0; address < 0x10000u; ++address)
        impl_->mem.write(static_cast<uint16_t>(address), fill);
}

void machine::load_binary(const std::filesystem::path& path, uint16_t origin) {
    const auto bytes = read_file_bytes(path);
    load_bytes(origin, std::span<const uint8_t>(bytes.data(), bytes.size()));
}

void machine::load_bytes(uint16_t origin, std::span<const uint8_t> bytes) noexcept {
    impl_->mem.load(origin, bytes);
}

std::vector<uint8_t> machine::read_memory(uint32_t address, std::size_t length) const {
    std::vector<uint8_t> bytes;
    bytes.reserve(length);
    for (std::size_t i = 0; i < length; ++i)
        bytes.push_back(read_byte(static_cast<uint16_t>((address + i) & 0xFFFF)));
    return bytes;
}

uint8_t machine::read_byte(uint16_t address) const noexcept {
    return impl_->mem.read(address);
}

void machine::write_memory(uint32_t address, std::span<const uint8_t> bytes) noexcept {
    for (std::size_t i = 0; i < bytes.size(); ++i)
        impl_->mem.write(static_cast<uint16_t>((address + i) & 0xFFFF), bytes[i]);
}

void machine::write_byte(uint16_t address, uint8_t value) noexcept {
    impl_->mem.write(address, value);
}

xz80::cpu_state machine::snapshot() const noexcept {
    return impl_->cpu.snapshot();
}

void machine::restore(const xz80::cpu_state& state) noexcept {
    impl_->cpu.restore(state);
}

register_image machine::registers() const noexcept {
    return image_from_state(snapshot());
}

void machine::set_registers(const register_image& regs) noexcept {
    impl_->cpu.restore(merge_state(impl_->cpu.snapshot(), regs));
}

void machine::set_pc(uint16_t pc) noexcept {
    impl_->cpu.set_reg(xz80::reg16::PC, pc);
}

void machine::set_sp(uint16_t sp) noexcept {
    impl_->cpu.set_reg(xz80::reg16::SP, sp);
}

void machine::bind_stdin(uint16_t port, std::istream& input) noexcept {
    impl_->ports.bind_stdin(port, input);
}

void machine::bind_stdin_status_data(
    uint16_t status_port,
    uint16_t data_port,
    std::istream& input) noexcept
{
    impl_->ports.bind_stdin_status_data(status_port, data_port, input);
}

void machine::bind_stdout(uint16_t port, std::ostream& output) noexcept {
    impl_->ports.bind_stdout(port, output);
}

void machine::clear_stdin() noexcept {
    impl_->ports.clear_stdin();
}

void machine::clear_stdout() noexcept {
    impl_->ports.clear_stdout();
}

void machine::clear_io() noexcept {
    clear_stdin();
    clear_stdout();
}

void machine::insert_breakpoint(uint16_t address) {
    if (!impl_->has_breakpoint(address))
        impl_->breakpoints.push_back(address);
}

void machine::remove_breakpoint(uint16_t address) {
    impl_->breakpoints.erase(
        std::remove(impl_->breakpoints.begin(), impl_->breakpoints.end(), address),
        impl_->breakpoints.end());
}

void machine::clear_breakpoints() noexcept {
    impl_->breakpoints.clear();
}

stop_result machine::continue_execution(std::size_t max_steps) noexcept {
    stop_result stop;
    stop.pc = impl_->cpu.pc();
    if (impl_->cpu.halted()) {
        stop.reason = stop_reason::halted;
        return stop;
    }

    for (std::size_t step = 0; step < max_steps; ++step) {
        if (impl_->has_breakpoint(impl_->cpu.pc())) {
            stop.reason = stop_reason::breakpoint;
            stop.steps = step;
            stop.pc = impl_->cpu.pc();
            return stop;
        }

        try {
            impl_->cpu.step();
        } catch (const std::exception& e) {
            stop.reason = stop_reason::fault;
            stop.steps = step;
            stop.pc = impl_->cpu.pc();
            stop.message = e.what();
            return stop;
        } catch (...) {
            stop.reason = stop_reason::fault;
            stop.steps = step;
            stop.pc = impl_->cpu.pc();
            stop.message = "unknown emulator fault";
            return stop;
        }
        if (impl_->cpu.halted()) {
            stop.reason = stop_reason::halted;
            stop.steps = step + 1;
            stop.pc = impl_->cpu.pc();
            return stop;
        }
    }

    stop.reason = stop_reason::step_limit;
    stop.steps = max_steps;
    stop.pc = impl_->cpu.pc();
    return stop;
}

stop_result machine::step_instruction() noexcept {
    stop_result stop;
    stop.pc = impl_->cpu.pc();
    if (impl_->cpu.halted()) {
        stop.reason = stop_reason::halted;
        return stop;
    }

    try {
        impl_->cpu.step();
    } catch (const std::exception& e) {
        stop.reason = stop_reason::fault;
        stop.message = e.what();
        stop.pc = impl_->cpu.pc();
        return stop;
    } catch (...) {
        stop.reason = stop_reason::fault;
        stop.message = "unknown emulator fault";
        stop.pc = impl_->cpu.pc();
        return stop;
    }
    stop.reason = impl_->cpu.halted() ? stop_reason::halted : stop_reason::stepped;
    stop.steps = 1;
    stop.pc = impl_->cpu.pc();
    return stop;
}

bool machine::halted() const noexcept {
    return impl_->cpu.halted();
}

rsp_target_adapter::rsp_target_adapter(machine& emu) noexcept
    : emu_(&emu)
{}

std::vector<uint8_t> rsp_target_adapter::read_registers() {
    const auto regs = emu_->registers();
    return pack_registers(regs);
}

void rsp_target_adapter::write_registers(const std::vector<uint8_t>& data) {
    emu_->set_registers(unpack_registers(data));
}

std::vector<uint8_t> rsp_target_adapter::read_memory(
    uint32_t address,
    std::size_t length)
{
    return emu_->read_memory(address, length);
}

void rsp_target_adapter::write_memory(
    uint32_t address,
    const std::vector<uint8_t>& data)
{
    emu_->write_memory(address, std::span<const uint8_t>(data.data(), data.size()));
}

std::string rsp_target_adapter::cont() {
    last_stop_ = to_rsp_stop(emu_->continue_execution());
    return last_stop_;
}

std::string rsp_target_adapter::step() {
    last_stop_ = to_rsp_stop(emu_->step_instruction());
    return last_stop_;
}

std::string rsp_target_adapter::stop_reason() {
    return last_stop_;
}

void rsp_target_adapter::insert_breakpoint(uint32_t address) {
    emu_->insert_breakpoint(static_cast<uint16_t>(address));
}

void rsp_target_adapter::remove_breakpoint(uint32_t address) {
    emu_->remove_breakpoint(static_cast<uint16_t>(address));
}

void rsp_target_adapter::detach() {}

void remote_session::connect(const std::string& host, uint16_t port) {
    client_.connect(host, port);
}

void remote_session::close() {
    client_.close();
}

bool remote_session::is_connected() const {
    return client_.is_connected();
}

rsp::stop_reply remote_session::query_stop() {
    return client_.query_stop();
}

register_image remote_session::read_registers() {
    return unpack_registers(client_.read_registers());
}

void remote_session::write_registers(const register_image& regs) {
    client_.write_registers(pack_registers(regs));
}

void remote_session::set_pc(uint16_t pc) {
    auto regs = read_registers();
    regs.pc = pc;
    write_registers(regs);
}

void remote_session::set_sp(uint16_t sp) {
    auto regs = read_registers();
    regs.sp = sp;
    write_registers(regs);
}

std::vector<uint8_t> remote_session::read_memory(uint32_t address, std::size_t length) {
    return client_.read_memory(address, length);
}

void remote_session::write_memory(uint32_t address, std::span<const uint8_t> data) {
    client_.write_memory(
        address,
        std::vector<uint8_t>(data.begin(), data.end()));
}

void remote_session::load_binary(const std::filesystem::path& path, uint16_t origin) {
    const auto bytes = read_file_bytes(path);
    write_memory(origin, std::span<const uint8_t>(bytes.data(), bytes.size()));
}

rsp::stop_reply remote_session::continue_execution() {
    return client_.cont();
}

rsp::stop_reply remote_session::step_instruction() {
    return client_.step();
}

void remote_session::pause() {
    client_.pause();
}

void remote_session::insert_breakpoint(uint32_t address) {
    client_.insert_breakpoint(address);
}

void remote_session::remove_breakpoint(uint32_t address) {
    client_.remove_breakpoint(address);
}

void remote_session::detach() {
    client_.detach();
}

} // namespace xemu
