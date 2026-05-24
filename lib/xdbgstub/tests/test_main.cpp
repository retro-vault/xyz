#include <chrono>
#include <cstdint>
#include <exception>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <xdbgstub/xdbgstub.hpp>

struct test_case {
    std::string name;
    std::function<void()> func;
};

static std::vector<test_case>& test_registry() {
    static std::vector<test_case> tests;
    return tests;
}

struct test_registrar {
    test_registrar(const std::string& name, std::function<void()> func) {
        test_registry().push_back({name, func});
    }
};

#define TEST(name) \
    static void test_##name(); \
    static test_registrar reg_##name(#name, test_##name); \
    static void test_##name()

#define ASSERT(cond) do { \
    if (!(cond)) { \
        std::cerr << "  FAIL: " << #cond \
                  << " (" << __FILE__ << ":" << __LINE__ << ")\n"; \
        throw std::runtime_error("assertion failed"); \
    } \
} while(0)

#define ASSERT_EQ(a, b) do { \
    const auto _a = (a); \
    const auto _b = (b); \
    if (!(_a == _b)) { \
        std::cerr << "  FAIL: " << #a << " == " << #b \
                  << " (" << __FILE__ << ":" << __LINE__ << ")\n"; \
        throw std::runtime_error("assertion failed"); \
    } \
} while(0)

class fake_target : public xdbgstub::target {
public:
    fake_target() {
        state_.pc = 0x0100;
        memory_.resize(0x2000, 0x00);
        memory_[0x0100] = 0x3E;
        memory_[0x0101] = 0x42;
        memory_[0x0102] = 0xC9;
    }

    xdbgstub::target_status status() override {
        xdbgstub::target_status status;
        status.state = running_ ? xdbgstub::execution_state::running
                                : xdbgstub::execution_state::stopped;
        status.reason = last_reason_;
        status.pc = state_.pc;
        status.registers = state_;
        return status;
    }

    xdbgstub::cpu_state read_registers() override {
        return state_;
    }

    void write_registers(const xdbgstub::cpu_state& state) override {
        state_ = state;
    }

    std::vector<uint8_t> read_memory(
        uint32_t address, std::size_t length) override
    {
        return std::vector<uint8_t>(
            memory_.begin() + address,
            memory_.begin() + address + length);
    }

    void write_memory(uint32_t address, const std::vector<uint8_t>& data) override {
        for (std::size_t i = 0; i < data.size(); ++i) {
            memory_[address + i] = data[i];
        }
    }

    xdbgstub::target_status continue_execution() override {
        running_ = false;
        if (breakpoint_.has_value() && breakpoint_.value() == state_.pc) {
            last_reason_ = xdbgstub::stop_reason::breakpoint;
        } else {
            state_.pc = static_cast<uint16_t>(state_.pc + 1);
            last_reason_ = xdbgstub::stop_reason::step;
        }
        auto result = status();
        result.reason = last_reason_;
        return result;
    }

    xdbgstub::target_status step_instruction() override {
        running_ = false;
        state_.pc = static_cast<uint16_t>(state_.pc + 1);
        last_reason_ = xdbgstub::stop_reason::step;
        return status();
    }

    xdbgstub::target_status pause_execution() override {
        running_ = false;
        last_reason_ = xdbgstub::stop_reason::pause;
        return status();
    }

    void set_breakpoint(uint32_t address) override {
        breakpoint_ = address;
    }

    void clear_breakpoint(uint32_t address) override {
        if (breakpoint_.has_value() && breakpoint_.value() == address) {
            breakpoint_.reset();
        }
    }

    void detach() override {
        detached_ = true;
    }

    bool detached() const {
        return detached_;
    }

private:
    xdbgstub::cpu_state state_{};
    std::vector<uint8_t> memory_;
    std::optional<uint32_t> breakpoint_;
    xdbgstub::stop_reason last_reason_ = xdbgstub::stop_reason::none;
    bool running_ = false;
    bool detached_ = false;
};

TEST(client_server_round_trip) {
    fake_target target;
    xdbgstub::server server;
    server.listen("127.0.0.1", 40123);

    std::thread worker([&]() {
        server.serve(target);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    xdbgstub::client client;
    client.connect("127.0.0.1", 40123);

    client.ping();

    auto regs = client.read_registers();
    ASSERT_EQ(regs.pc, 0x0100);

    auto mem = client.read_memory(0x0100, 3);
    ASSERT_EQ(mem.size(), 3u);
    ASSERT_EQ(mem[0], 0x3E);
    ASSERT_EQ(mem[2], 0xC9);

    client.set_breakpoint(0x0100);
    auto stop = client.continue_execution();
    ASSERT_EQ(stop.reason, xdbgstub::stop_reason::breakpoint);
    ASSERT_EQ(stop.pc, 0x0100u);

    stop = client.step_instruction();
    ASSERT_EQ(stop.reason, xdbgstub::stop_reason::step);
    ASSERT_EQ(stop.pc, 0x0101u);

    client.write_memory(0x0101, {0x99});
    mem = client.read_memory(0x0100, 3);
    ASSERT_EQ(mem[1], 0x99);

    regs.pc = 0x0200;
    client.write_registers(regs);
    ASSERT_EQ(client.read_registers().pc, 0x0200);

    client.detach();
    worker.join();
    ASSERT(target.detached());
}

TEST(server_close_unblocks_accept) {
    fake_target target;
    xdbgstub::server server;
    server.listen("127.0.0.1", 40124);

    std::exception_ptr worker_failure;
    std::thread worker([&]() {
        try {
            server.serve(target);
        } catch (...) {
            worker_failure = std::current_exception();
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    server.close();
    worker.join();

    ASSERT(!server.is_listening());
    ASSERT(worker_failure == nullptr);
}

TEST(server_close_unblocks_idle_client) {
    fake_target target;
    xdbgstub::server server;
    server.listen("127.0.0.1", 40125);

    std::exception_ptr worker_failure;
    std::thread worker([&]() {
        try {
            server.serve(target);
        } catch (...) {
            worker_failure = std::current_exception();
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    xdbgstub::client client;
    client.connect("127.0.0.1", 40125);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    server.close();
    worker.join();
    client.close();

    ASSERT(!server.is_listening());
    ASSERT(worker_failure == nullptr);
}

int main() {
    int passed = 0;
    int failed = 0;

    for (const auto& test : test_registry()) {
        std::cout << "  " << test.name << "... ";
        try {
            test.func();
            std::cout << "OK\n";
            ++passed;
        } catch (const std::exception&) {
            std::cout << "FAILED\n";
            ++failed;
        }
    }

    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
