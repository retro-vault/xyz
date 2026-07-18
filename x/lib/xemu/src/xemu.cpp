// xemu.cpp — host-side Z80 emulator library implementation.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih

#include <algorithm>
#include <array>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <fstream>
#include <ios>
#include <istream>
#include <memory>
#include <optional>
#include <ostream>
#include <span>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

#include <rsp/rsp.h>
#include <xbfd/xbfd.h>
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

uint16_t load_elf_sections(
    const std::filesystem::path& path,
    const std::function<void(uint16_t, std::span<const uint8_t>)>& load)
{
    try {
        auto obj = bfd::bfd::open_r(path);
        if (obj->get_flavour() != bfd::flavour::elf
            || (!obj->check_format(bfd::format::object)
                && !obj->check_format(bfd::format::executable))) {
            throw std::runtime_error("not an ELF object/executable: "
                                     + path.string());
        }

        for (const auto& sec : obj->sections()) {
            if (!xbfd::has_flag(sec.flags, xbfd::section_flags::alloc))
                continue;
            if (xbfd::has_flag(sec.flags, xbfd::section_flags::debugging))
                continue;
            if (sec.data.empty())
                continue;
            if (sec.vma > 0xFFFFu || sec.vma + sec.data.size() > 0x10000u) {
                throw std::runtime_error("ELF section out of Z80 address range: "
                                         + sec.name);
            }
            load(static_cast<uint16_t>(sec.vma),
                 std::span<const uint8_t>(sec.data.data(), sec.data.size()));
        }

        if (obj->object().entry > 0xFFFFu)
            throw std::runtime_error("ELF entry is outside Z80 address range");
        return static_cast<uint16_t>(obj->object().entry);
    } catch (const xbfd::bfd_error& e) {
        throw std::runtime_error("cannot load ELF: "
                                 + path.string() + ": " + e.what());
    }
}

struct ihx_chunk {
    uint16_t address = 0;
    std::vector<uint8_t> bytes;
};

int hex_nibble(char ch) {
    if (ch >= '0' && ch <= '9')
        return ch - '0';
    if (ch >= 'a' && ch <= 'f')
        return 10 + (ch - 'a');
    if (ch >= 'A' && ch <= 'F')
        return 10 + (ch - 'A');
    throw std::runtime_error("bad Intel HEX digit");
}

uint8_t parse_hex_byte(const std::string& line, std::size_t offset) {
    return static_cast<uint8_t>((hex_nibble(line.at(offset)) << 4)
                                | hex_nibble(line.at(offset + 1)));
}

std::vector<ihx_chunk> read_ihx_chunks(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input.is_open())
        throw std::runtime_error("cannot open Intel HEX: " + path.string());

    std::vector<ihx_chunk> chunks;
    std::string line;
    uint32_t ext_base = 0;

    while (std::getline(input, line)) {
        if (line.empty())
            continue;
        if (line[0] != ':')
            throw std::runtime_error("invalid Intel HEX record in " + path.string());
        if (line.size() < 11)
            throw std::runtime_error("truncated Intel HEX record in " + path.string());

        const uint8_t len = parse_hex_byte(line, 1);
        const std::size_t expected_size = 11u + static_cast<std::size_t>(len) * 2u;
        if (line.size() < expected_size)
            throw std::runtime_error("short Intel HEX record in " + path.string());

        const uint16_t addr = static_cast<uint16_t>(
            (parse_hex_byte(line, 3) << 8) | parse_hex_byte(line, 5));
        const uint8_t type = parse_hex_byte(line, 7);
        uint8_t checksum = static_cast<uint8_t>(
            len + static_cast<uint8_t>((addr >> 8) & 0xff)
            + static_cast<uint8_t>(addr & 0xff) + type);

        std::vector<uint8_t> bytes;
        bytes.reserve(len);
        for (uint8_t i = 0; i < len; ++i) {
            const uint8_t byte = parse_hex_byte(line, 9 + i * 2);
            checksum = static_cast<uint8_t>(checksum + byte);
            bytes.push_back(byte);
        }
        checksum = static_cast<uint8_t>(checksum + parse_hex_byte(line, 9 + len * 2));
        if (checksum != 0)
            throw std::runtime_error("bad Intel HEX checksum in " + path.string());

        if (type == 0x00) {
            const uint32_t full = ext_base + addr;
            if (full + bytes.size() > 0x10000u)
                throw std::runtime_error("Intel HEX image exceeds 64K: " + path.string());

            if (!chunks.empty()
                && static_cast<uint32_t>(chunks.back().address + chunks.back().bytes.size())
                       == full) {
                chunks.back().bytes.insert(
                    chunks.back().bytes.end(),
                    bytes.begin(),
                    bytes.end());
            } else {
                chunks.push_back({static_cast<uint16_t>(full), std::move(bytes)});
            }
        } else if (type == 0x01) {
            break;
        } else if (type == 0x04) {
            if (bytes.size() != 2)
                throw std::runtime_error("bad Intel HEX extended address in " + path.string());
            ext_base = static_cast<uint32_t>(
                (static_cast<uint32_t>(bytes[0]) << 24)
                | (static_cast<uint32_t>(bytes[1]) << 16));
        }
    }

    return chunks;
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

class mapped_memory final : public xz80::IMemory {
public:
    static constexpr std::size_t compat_page_count = banked_memory_config::page_count;
    static constexpr uint32_t compat_page_size = banked_memory_config::page_size;

    explicit mapped_memory(uint8_t fill = 0x00)
        : fill_(fill)
    {
        configure(default_flat_config());
    }

    uint8_t read(uint16_t addr) const noexcept override {
        const int window_index = addr_to_window_[addr];
        if (window_index < 0) {
            return fill_;
        }

        const auto& window = windows_[static_cast<std::size_t>(window_index)];
        const auto& store = stores_[window.store_index];
        const auto bank = resolve_bank(window, store);
        const auto offset = static_cast<uint32_t>(addr - window.start);
        return store.data[data_index(store, bank, window.bank_offset + offset)];
    }

    void write(uint16_t addr, uint8_t value) noexcept override {
        store_byte(addr, value, true);
    }

    void clear(uint8_t fill) noexcept {
        fill_ = fill;
        for (auto& store : stores_) {
            std::fill(store.data.begin(), store.data.end(), fill);
        }
        reset_selectors();
    }

    void reset_selectors() noexcept {
        for (auto& selector : selectors_) {
            selector.value = selector.initial_value;
        }
    }

    void load(uint16_t origin, std::span<const uint8_t> src) noexcept {
        for (std::size_t i = 0; i < src.size(); ++i) {
            const auto addr = static_cast<uint32_t>(origin) + static_cast<uint32_t>(i);
            if (addr >= 0x10000u) {
                break;
            }
            store_byte(static_cast<uint16_t>(addr), src[i], false);
        }
    }

    void configure(const memory_map_config& config) {
        if (config.stores.empty()) {
            throw std::runtime_error("memory map must define at least one store");
        }
        if (config.windows.empty()) {
            throw std::runtime_error("memory map must define at least one window");
        }

        std::unordered_map<std::string, std::size_t> store_lookup;
        std::unordered_map<std::string, std::size_t> selector_lookup;
        std::vector<store_runtime> new_stores;
        std::vector<selector_runtime> new_selectors;
        std::vector<window_runtime> new_windows;
        std::vector<port_rule_runtime> new_port_rules;
        std::array<int, 0x10000> new_addr_to_window{};
        new_addr_to_window.fill(-1);

        for (const auto& store_cfg : config.stores) {
            if (store_cfg.name.empty()) {
                throw std::runtime_error("memory store name cannot be empty");
            }
            if (store_lookup.count(store_cfg.name) != 0) {
                throw std::runtime_error("duplicate memory store: " + store_cfg.name);
            }
            if (store_cfg.bank_count == 0) {
                throw std::runtime_error("store bank_count must be at least 1: " + store_cfg.name);
            }
            if (store_cfg.bank_size == 0) {
                throw std::runtime_error("store bank_size must be at least 1: " + store_cfg.name);
            }

            const auto index = new_stores.size();
            store_lookup[store_cfg.name] = index;
            auto& store = new_stores.emplace_back();
            store.name = store_cfg.name;
            store.bank_count = store_cfg.bank_count;
            store.bank_size = store_cfg.bank_size;
            store.writable = store_cfg.writable;
            store.data.assign(
                static_cast<std::size_t>(store_cfg.bank_count)
                    * static_cast<std::size_t>(store_cfg.bank_size),
                fill_);
        }

        for (const auto& selector_cfg : config.selectors) {
            if (selector_cfg.name.empty()) {
                throw std::runtime_error("memory selector name cannot be empty");
            }
            if (selector_lookup.count(selector_cfg.name) != 0) {
                throw std::runtime_error("duplicate selector: " + selector_cfg.name);
            }

            const auto index = new_selectors.size();
            selector_lookup[selector_cfg.name] = index;
            auto& selector = new_selectors.emplace_back();
            selector.name = selector_cfg.name;
            selector.initial_value = selector_cfg.initial_value;
            selector.value = selector_cfg.initial_value;
        }

        for (const auto& window_cfg : config.windows) {
            if (window_cfg.store.empty()) {
                throw std::runtime_error("memory window store cannot be empty");
            }
            const auto store_it = store_lookup.find(window_cfg.store);
            if (store_it == store_lookup.end()) {
                throw std::runtime_error("unknown window store: " + window_cfg.store);
            }
            if (window_cfg.end < window_cfg.start) {
                throw std::runtime_error("window end before start for store " + window_cfg.store);
            }
            if (window_cfg.fixed_bank.has_value() && window_cfg.selector.has_value()) {
                throw std::runtime_error(
                    "window cannot specify both fixed_bank and selector for store "
                    + window_cfg.store);
            }

            const auto range_size = static_cast<uint32_t>(
                static_cast<uint32_t>(window_cfg.end) - static_cast<uint32_t>(window_cfg.start)
                + 1u);
            const auto& store = new_stores[store_it->second];
            if (window_cfg.bank_offset + range_size > store.bank_size) {
                throw std::runtime_error(
                    "window range exceeds bank size for store " + window_cfg.store);
            }
            if (window_cfg.fixed_bank.has_value()
                && window_cfg.fixed_bank.value() >= store.bank_count) {
                throw std::runtime_error(
                    "fixed bank out of range for store " + window_cfg.store);
            }

            int selector_index = -1;
            if (window_cfg.selector.has_value()) {
                const auto selector_it = selector_lookup.find(window_cfg.selector.value());
                if (selector_it == selector_lookup.end()) {
                    throw std::runtime_error(
                        "unknown selector " + window_cfg.selector.value()
                        + " for store " + window_cfg.store);
                }
                selector_index = static_cast<int>(selector_it->second);
            }

            const auto window_index = static_cast<int>(new_windows.size());
            auto& window = new_windows.emplace_back();
            window.start = window_cfg.start;
            window.end = window_cfg.end;
            window.store_index = store_it->second;
            window.fixed_bank = window_cfg.fixed_bank.value_or(0);
            window.selector_index = selector_index;
            window.bank_offset = window_cfg.bank_offset;

            for (uint32_t addr = window.start; addr <= window.end; ++addr) {
                auto& slot = new_addr_to_window[static_cast<std::size_t>(addr)];
                if (slot >= 0) {
                    throw std::runtime_error("memory windows overlap at address 0x"
                                             + hex_word(static_cast<uint16_t>(addr)));
                }
                slot = window_index;
            }
        }

        for (const auto& rule_cfg : config.port_rules) {
            const auto selector_it = selector_lookup.find(rule_cfg.selector);
            if (selector_it == selector_lookup.end()) {
                throw std::runtime_error("unknown selector in port rule: " + rule_cfg.selector);
            }
            if (rule_cfg.shift > 15) {
                throw std::runtime_error("port rule shift must be in the range 0..15");
            }

            auto& rule = new_port_rules.emplace_back();
            rule.port = rule_cfg.port;
            rule.port_mask = rule_cfg.port_mask;
            rule.selector_index = selector_it->second;
            rule.mask = rule_cfg.mask;
            rule.shift = rule_cfg.shift;
        }

        stores_ = std::move(new_stores);
        selectors_ = std::move(new_selectors);
        windows_ = std::move(new_windows);
        port_rules_ = std::move(new_port_rules);
        addr_to_window_ = new_addr_to_window;
        compat_selector_index_.reset();
    }

    void configure_compat(const banked_memory_config& config) {
        validate_compat_config(config);

        memory_map_config map;
        const bool have_banked_pages = !config.banked_pages.empty();
        constexpr const char* compat_selector_name = "__compat_bank";

        if (have_banked_pages) {
            map.selectors.push_back({compat_selector_name, 0});
        }

        std::array<bool, compat_page_count> banked_mask{};
        for (const auto page : config.banked_pages) {
            banked_mask[page] = true;
        }

        for (uint8_t page = 0; page < compat_page_count; ++page) {
            memory_store_config store;
            store.name = "compat_page_" + std::to_string(page);
            store.bank_count = banked_mask[page] ? config.bank_count : 1;
            store.bank_size = compat_page_size;
            store.writable = true;
            map.stores.push_back(store);

            memory_window_config window;
            window.start = static_cast<uint16_t>(page * compat_page_size);
            window.end = static_cast<uint16_t>(window.start + compat_page_size - 1u);
            window.store = store.name;
            if (banked_mask[page]) {
                window.selector = compat_selector_name;
            } else {
                window.fixed_bank = 0;
            }
            map.windows.push_back(window);
        }

        configure(map);
        compat_selector_index_ = find_selector_index(compat_selector_name);
    }

    void apply_port_write(uint16_t port, uint8_t value) noexcept {
        for (const auto& rule : port_rules_) {
            if ((port & rule.port_mask) != (rule.port & rule.port_mask)) {
                continue;
            }
            selectors_[rule.selector_index].value = static_cast<uint16_t>(
                (static_cast<uint16_t>(value) & rule.mask) >> rule.shift);
        }
    }

    void set_compat_active_bank(uint16_t bank) noexcept {
        if (compat_selector_index_.has_value()) {
            selectors_[compat_selector_index_.value()].value = bank;
        }
    }

    uint16_t compat_active_bank() const noexcept {
        if (compat_selector_index_.has_value()) {
            return selectors_[compat_selector_index_.value()].value;
        }
        return 0;
    }

private:
    void store_byte(uint16_t addr, uint8_t value, bool honor_writable) noexcept {
        const int window_index = addr_to_window_[addr];
        if (window_index < 0) {
            return;
        }

        const auto& window = windows_[static_cast<std::size_t>(window_index)];
        auto& store = stores_[window.store_index];
        if (honor_writable && !store.writable) {
            return;
        }

        const auto bank = resolve_bank(window, store);
        const auto offset = static_cast<uint32_t>(addr - window.start);
        store.data[data_index(store, bank, window.bank_offset + offset)] = value;
    }

    struct store_runtime {
        std::string name;
        uint16_t bank_count = 1;
        uint32_t bank_size = 0x10000;
        bool writable = true;
        std::vector<uint8_t> data;
    };

    struct selector_runtime {
        std::string name;
        uint16_t initial_value = 0;
        uint16_t value = 0;
    };

    struct window_runtime {
        uint16_t start = 0;
        uint16_t end = 0xFFFF;
        std::size_t store_index = 0;
        uint16_t fixed_bank = 0;
        int selector_index = -1;
        uint32_t bank_offset = 0;
    };

    struct port_rule_runtime {
        uint16_t port = 0;
        uint16_t port_mask = 0xFFFF;
        std::size_t selector_index = 0;
        uint16_t mask = 0x00FF;
        uint8_t shift = 0;
    };

    static memory_map_config default_flat_config() {
        memory_map_config config;
        config.stores.push_back({"main", 1, 0x10000u, true});
        config.windows.push_back({0x0000, 0xFFFF, "main", 0, std::nullopt, 0});
        return config;
    }

    static std::string hex_word(uint16_t value) {
        static constexpr char digits[] = "0123456789ABCDEF";
        std::string text(4, '0');
        text[0] = digits[(value >> 12) & 0x0F];
        text[1] = digits[(value >> 8) & 0x0F];
        text[2] = digits[(value >> 4) & 0x0F];
        text[3] = digits[value & 0x0F];
        return text;
    }

    static void validate_compat_config(const banked_memory_config& config) {
        std::array<bool, compat_page_count> seen{};
        auto mark_pages = [&](const std::vector<uint8_t>& pages, const char* label) {
            for (const auto page_value : pages) {
                if (page_value >= compat_page_count) {
                    throw std::runtime_error(
                        std::string(label) + " contains invalid page index "
                        + std::to_string(page_value));
                }
                if (seen[page_value]) {
                    throw std::runtime_error(
                        std::string("duplicate page index ")
                        + std::to_string(page_value) + " in banked memory configuration");
                }
                seen[page_value] = true;
            }
        };

        mark_pages(config.shared_pages, "shared_pages");
        mark_pages(config.banked_pages, "banked_pages");

        if (config.shared_pages.size() + config.banked_pages.size() != compat_page_count) {
            throw std::runtime_error(
                "banked memory configuration must classify all 4 CPU pages");
        }

        if (config.banked_pages.empty()) {
            if (config.bank_count != 0) {
                throw std::runtime_error(
                    "bank_count must be 0 when no banked pages are configured");
            }
            return;
        }

        if (config.bank_count == 0 || config.bank_count > 256) {
            throw std::runtime_error("bank_count must be in the range 1..256");
        }
    }

    uint16_t resolve_bank(const window_runtime& window, const store_runtime& store) const noexcept {
        uint16_t bank = window.fixed_bank;
        if (window.selector_index >= 0) {
            bank = selectors_[static_cast<std::size_t>(window.selector_index)].value;
        }
        if (store.bank_count == 0) {
            return 0;
        }
        return static_cast<uint16_t>(bank % store.bank_count);
    }

    std::size_t data_index(
        const store_runtime& store,
        uint16_t bank,
        uint32_t offset) const noexcept
    {
        return static_cast<std::size_t>(bank) * static_cast<std::size_t>(store.bank_size)
            + static_cast<std::size_t>(offset);
    }

    std::optional<int> find_selector_index(const std::string& name) const noexcept {
        for (std::size_t i = 0; i < selectors_.size(); ++i) {
            if (selectors_[i].name == name) {
                return static_cast<int>(i);
            }
        }
        return std::nullopt;
    }

    uint8_t fill_ = 0x00;
    std::vector<store_runtime> stores_;
    std::vector<selector_runtime> selectors_;
    std::vector<window_runtime> windows_;
    std::vector<port_rule_runtime> port_rules_;
    std::array<int, 0x10000> addr_to_window_{};
    std::optional<int> compat_selector_index_;
};

constexpr uint16_t kEmuReqFd = 0xff11;
constexpr uint16_t kEmuReqPtr = 0xff13;
constexpr uint16_t kEmuReqLen = 0xff15;
constexpr uint16_t kEmuReqFlags = 0xff17;
constexpr uint16_t kEmuReqMode = 0xff19;
constexpr uint16_t kEmuReqWhence = 0xff1b;
constexpr uint16_t kEmuReqOffset = 0xff1d;
constexpr uint16_t kEmuReqPath = 0xff21;
constexpr uint16_t kEmuReqPath2 = 0xff23;
constexpr uint16_t kEmuReqResult = 0xff25;

constexpr uint8_t kEmuCmdExit = 1;
constexpr uint8_t kEmuCmdOpen = 4;
constexpr uint8_t kEmuCmdClose = 5;
constexpr uint8_t kEmuCmdRead = 6;
constexpr uint8_t kEmuCmdWrite = 7;
constexpr uint8_t kEmuCmdLseek = 8;
constexpr uint8_t kEmuCmdUnlink = 9;
constexpr uint8_t kEmuCmdRename = 10;

constexpr uint16_t kOpenAccessMask = 0x0003;
constexpr uint16_t kOpenWriteOnly = 0x0001;
constexpr uint16_t kOpenReadWrite = 0x0002;
constexpr uint16_t kOpenCreate = 0x0100;
constexpr uint16_t kOpenTruncate = 0x0200;
constexpr uint16_t kOpenAppend = 0x0400;

} // namespace

struct machine::impl {
    struct file_handle {
        std::fstream stream;
        uint16_t flags = 0;
    };

    class port_mux final : public xz80::IPorts {
    public:
        explicit port_mux(impl& owner) noexcept
            : owner_(&owner)
        {}

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
            if (owner_ != nullptr) {
                owner_->mem.apply_port_write(port, value);
            }
            if (bank_port_.has_value() && port == bank_port_.value() && owner_ != nullptr) {
                owner_->mem.set_compat_active_bank(value);
            }
            if (port == machine::emu_cmd_port && owner_ != nullptr) {
                owner_->handle_command(value);
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

        void bind_bank_port(uint16_t port) noexcept {
            bank_port_ = port;
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

        void clear_bank_port() noexcept {
            bank_port_.reset();
        }

        std::istream* stdin_stream() const noexcept {
            return stdin_stream_;
        }

        std::ostream* stdout_stream() const noexcept {
            return stdout_stream_;
        }

    private:
        impl* owner_ = nullptr;
        std::optional<uint16_t> stdin_status_port_;
        std::optional<uint16_t> stdin_data_port_;
        std::optional<uint16_t> stdout_port_;
        std::optional<uint16_t> bank_port_;
        std::istream* stdin_stream_ = nullptr;
        std::ostream* stdout_stream_ = nullptr;
    };

    explicit impl(uint8_t fill)
        : ports(*this)
        , mem(fill)
        , cpu(mem, ports)
    {
        cpu.reset();
    }

    bool has_breakpoint(uint16_t address) const noexcept {
        return std::find(breakpoints.begin(), breakpoints.end(), address) !=
            breakpoints.end();
    }

    uint8_t read8(uint16_t address) const noexcept {
        return mem.read(address);
    }

    uint16_t read16(uint16_t address) const noexcept {
        return static_cast<uint16_t>(read8(address))
            | static_cast<uint16_t>(
                static_cast<uint16_t>(read8(static_cast<uint16_t>(address + 1u))) << 8);
    }

    uint32_t read32(uint16_t address) const noexcept {
        return static_cast<uint32_t>(read16(address))
            | (static_cast<uint32_t>(
                   read16(static_cast<uint16_t>(address + 2u)))
               << 16);
    }

    void write8(uint16_t address, uint8_t value) noexcept {
        mem.write(address, value);
    }

    void write16(uint16_t address, int value) noexcept {
        const auto uvalue = static_cast<uint16_t>(value);
        write8(address, static_cast<uint8_t>(uvalue & 0xff));
        write8(
            static_cast<uint16_t>(address + 1u),
            static_cast<uint8_t>((uvalue >> 8) & 0xff));
    }

    void write32(uint16_t address, std::int32_t value) noexcept {
        const auto uvalue = static_cast<std::uint32_t>(value);
        write16(address, static_cast<int>(uvalue & 0xffffu));
        write16(
            static_cast<uint16_t>(address + 2u),
            static_cast<int>((uvalue >> 16) & 0xffffu));
    }

    void write_result16(int value) noexcept {
        write16(kEmuReqResult, value);
        write16(kEmuReqResult + 2u, value < 0 ? -1 : 0);
    }

    void write_result32(std::int32_t value) noexcept {
        write32(kEmuReqResult, value);
    }

    std::string read_c_string(uint16_t address) const {
        std::string text;
        text.reserve(64);
        for (int i = 0; i < 1024; ++i) {
            const auto ch = read8(static_cast<uint16_t>(address + i));
            if (ch == 0) {
                return text;
            }
            text.push_back(static_cast<char>(ch));
        }
        return {};
    }

    std::filesystem::path resolve_guest_path(const std::string& guest_path) const {
        if (!host_fs_root_.has_value()) {
            return {};
        }
        if (guest_path.empty()) {
            return {};
        }

        const std::filesystem::path guest(guest_path);
        if (guest.is_absolute()) {
            return {};
        }

        const auto normalized = guest.lexically_normal();
        for (const auto& part : normalized) {
            if (part == "..") {
                return {};
            }
        }

        return *host_fs_root_ / normalized;
    }

    int alloc_fd(std::unique_ptr<file_handle> file) {
        for (std::size_t i = 0; i < files.size(); ++i) {
            if (!files[i]) {
                files[i] = std::move(file);
                return static_cast<int>(i) + 3;
            }
        }

        files.push_back(std::move(file));
        return static_cast<int>(files.size()) + 2;
    }

    file_handle* get_file(uint16_t fd) noexcept {
        if (fd < 3) {
            return nullptr;
        }

        const auto index = static_cast<std::size_t>(fd - 3);
        if (index >= files.size() || !files[index]) {
            return nullptr;
        }
        return files[index].get();
    }

    static std::streamoff end_position(file_handle& file) {
        const auto access = file.flags & kOpenAccessMask;
        const bool can_read = (access == 0) || (access == kOpenReadWrite);
        const bool can_write = (access == kOpenWriteOnly) || (access == kOpenReadWrite);

        file.stream.clear();
        if (can_read) {
            file.stream.seekg(0, std::ios::end);
            const auto pos = file.stream.tellg();
            if (pos >= 0) {
                return static_cast<std::streamoff>(pos);
            }
        }
        if (can_write) {
            file.stream.seekp(0, std::ios::end);
            const auto pos = file.stream.tellp();
            if (pos >= 0) {
                return static_cast<std::streamoff>(pos);
            }
        }
        return static_cast<std::streamoff>(-1);
    }

    static std::streamoff current_position(file_handle& file) {
        const auto access = file.flags & kOpenAccessMask;
        const bool can_read = (access == 0) || (access == kOpenReadWrite);
        const bool can_write = (access == kOpenWriteOnly) || (access == kOpenReadWrite);

        file.stream.clear();
        if (can_read) {
            const auto pos = file.stream.tellg();
            if (pos >= 0) {
                return static_cast<std::streamoff>(pos);
            }
        }
        if (can_write) {
            const auto pos = file.stream.tellp();
            if (pos >= 0) {
                return static_cast<std::streamoff>(pos);
            }
        }
        return static_cast<std::streamoff>(-1);
    }

    void handle_open() {
        const auto path = resolve_guest_path(read_c_string(read16(kEmuReqPath)));
        const auto flags = read16(kEmuReqFlags);
        if (path.empty()) {
            write_result16(-1);
            return;
        }

        std::ios::openmode mode = std::ios::binary;
        const auto access = static_cast<uint16_t>(flags & kOpenAccessMask);
        if (access == kOpenWriteOnly) {
            mode |= std::ios::out;
        } else if (access == kOpenReadWrite) {
            mode |= std::ios::in | std::ios::out;
        } else {
            mode |= std::ios::in;
        }
        if (flags & kOpenTruncate) {
            mode |= std::ios::trunc;
        }
        if (flags & kOpenAppend) {
            mode |= std::ios::app;
        }

        if (flags & kOpenCreate) {
            if (!path.parent_path().empty()) {
                std::filesystem::create_directories(path.parent_path());
            }
            std::ofstream create(path, std::ios::binary | std::ios::app);
        }

        auto file = std::make_unique<file_handle>();
        file->flags = flags;
        file->stream.open(path, mode);
        if (!file->stream && (flags & kOpenCreate) && access == kOpenReadWrite) {
            std::ofstream create(path, std::ios::binary);
            create.close();
            file->stream.clear();
            file->stream.open(path, mode);
        }
        if (!file->stream) {
            write_result16(-1);
            return;
        }

        if (flags & kOpenAppend) {
            file->stream.seekg(0, std::ios::end);
            file->stream.seekp(0, std::ios::end);
        }

        write_result16(alloc_fd(std::move(file)));
    }

    void handle_close() noexcept {
        const auto fd = read16(kEmuReqFd);
        if (fd < 3) {
            write_result16(0);
            return;
        }

        const auto index = static_cast<std::size_t>(fd - 3);
        if (index >= files.size() || !files[index]) {
            write_result16(-1);
            return;
        }

        files[index]->stream.close();
        files[index].reset();
        write_result16(0);
    }

    void handle_read() noexcept {
        const auto fd = read16(kEmuReqFd);
        const auto ptr = read16(kEmuReqPtr);
        const auto len = read16(kEmuReqLen);

        if (fd == 0) {
            auto* input = ports.stdin_stream();
            if (input == nullptr) {
                write_result16(0);
                return;
            }

            uint16_t count = 0;
            while (count < len) {
                const int ch = input->get();
                if (ch == std::char_traits<char>::eof()) {
                    input->clear();
                    break;
                }
                write8(static_cast<uint16_t>(ptr + count), static_cast<uint8_t>(ch));
                ++count;
            }

            write_result16(count);
            return;
        }

        auto* file = get_file(fd);
        if (file == nullptr) {
            write_result16(-1);
            return;
        }

        std::vector<char> buffer(len);
        file->stream.read(buffer.data(), static_cast<std::streamsize>(len));
        const auto count = static_cast<uint16_t>(file->stream.gcount());
        for (uint16_t i = 0; i < count; ++i) {
            write8(static_cast<uint16_t>(ptr + i), static_cast<uint8_t>(buffer[i]));
        }
        if (file->stream.eof()) {
            file->stream.clear();
        }
        write_result16(count);
    }

    void handle_write() noexcept {
        const auto fd = read16(kEmuReqFd);
        const auto ptr = read16(kEmuReqPtr);
        const auto len = read16(kEmuReqLen);

        if (fd == 1 || fd == 2) {
            if (auto* output = ports.stdout_stream(); output != nullptr) {
                for (uint16_t i = 0; i < len; ++i) {
                    output->put(static_cast<char>(read8(static_cast<uint16_t>(ptr + i))));
                }
                output->flush();
            }
            write_result16(len);
            return;
        }

        auto* file = get_file(fd);
        if (file == nullptr) {
            write_result16(-1);
            return;
        }

        for (uint16_t i = 0; i < len; ++i) {
            file->stream.put(static_cast<char>(read8(static_cast<uint16_t>(ptr + i))));
        }
        file->stream.flush();
        write_result16(file->stream ? len : -1);
    }

    void handle_lseek() noexcept {
        const auto fd = read16(kEmuReqFd);
        auto* file = get_file(fd);
        if (file == nullptr) {
            write_result32(-1);
            return;
        }

        const auto offset = static_cast<std::int32_t>(read32(kEmuReqOffset));
        const auto whence = read16(kEmuReqWhence);
        const auto access = static_cast<uint16_t>(file->flags & kOpenAccessMask);
        const bool can_read = (access == 0) || (access == kOpenReadWrite);
        const bool can_write = (access == kOpenWriteOnly) || (access == kOpenReadWrite);

        file->stream.clear();

        std::streamoff base = 0;
        if (whence == 0) {
            base = 0;
        } else if (whence == 1) {
            base = current_position(*file);
        } else if (whence == 2) {
            base = end_position(*file);
        } else {
            write_result32(-1);
            return;
        }

        if (base < 0) {
            write_result32(-1);
            return;
        }

        const auto target = base + static_cast<std::streamoff>(offset);
        if (target < 0) {
            write_result32(-1);
            return;
        }

        if (can_read) {
            file->stream.seekg(target, std::ios::beg);
        }
        if (can_write) {
            file->stream.seekp(target, std::ios::beg);
        }

        const auto pos = current_position(*file);
        if (pos < 0) {
            write_result32(-1);
            return;
        }

        write_result32(static_cast<std::int32_t>(pos));
    }

    void handle_unlink() {
        const auto path = resolve_guest_path(read_c_string(read16(kEmuReqPath)));
        if (path.empty()) {
            write_result16(-1);
            return;
        }

        std::error_code ec;
        const bool removed = std::filesystem::remove(path, ec);
        write_result16(!ec && removed ? 0 : -1);
    }

    void handle_rename() {
        const auto from = resolve_guest_path(read_c_string(read16(kEmuReqPath)));
        const auto to = resolve_guest_path(read_c_string(read16(kEmuReqPath2)));
        if (from.empty() || to.empty()) {
            write_result16(-1);
            return;
        }

        if (!to.parent_path().empty()) {
            std::filesystem::create_directories(to.parent_path());
        }
        std::error_code ec;
        std::filesystem::rename(from, to, ec);
        write_result16(!ec ? 0 : -1);
    }

    void handle_command(uint8_t value) noexcept {
        try {
            switch (value) {
            case kEmuCmdExit:
                break;
            case kEmuCmdOpen:
                handle_open();
                break;
            case kEmuCmdClose:
                handle_close();
                break;
            case kEmuCmdRead:
                handle_read();
                break;
            case kEmuCmdWrite:
                handle_write();
                break;
            case kEmuCmdLseek:
                handle_lseek();
                break;
            case kEmuCmdUnlink:
                handle_unlink();
                break;
            case kEmuCmdRename:
                handle_rename();
                break;
            default:
                break;
            }
        } catch (...) {
            if (value == kEmuCmdLseek) {
                write_result32(-1);
            } else if (value != kEmuCmdExit) {
                write_result16(-1);
            }
        }
    }

    port_mux ports;
    mapped_memory mem;
    xz80::cpu cpu;
    std::vector<uint16_t> breakpoints;
    std::optional<std::filesystem::path> host_fs_root_;
    std::vector<std::unique_ptr<file_handle>> files;
};

machine::machine(uint8_t fill)
    : impl_(std::make_unique<impl>(fill))
{}

machine::~machine() = default;
machine::machine(machine&&) noexcept = default;
machine& machine::operator=(machine&&) noexcept = default;

void machine::reset() noexcept {
    impl_->mem.reset_selectors();
    impl_->cpu.reset();
}

void machine::clear_memory(uint8_t fill) noexcept {
    impl_->mem.clear(fill);
}

void machine::load_binary(const std::filesystem::path& path, uint16_t origin) {
    const auto bytes = read_file_bytes(path);
    load_bytes(origin, std::span<const uint8_t>(bytes.data(), bytes.size()));
}

void machine::load_ihx(const std::filesystem::path& path) {
    for (const auto& chunk : read_ihx_chunks(path))
        load_bytes(chunk.address,
                   std::span<const uint8_t>(chunk.bytes.data(), chunk.bytes.size()));
}

uint16_t machine::load_elf(const std::filesystem::path& path) {
    return load_elf_sections(
        path,
        [this](uint16_t address, std::span<const uint8_t> bytes) {
            load_bytes(address, bytes);
        });
}

void machine::load_bytes(uint16_t origin, std::span<const uint8_t> bytes) noexcept {
    impl_->mem.load(origin, bytes);
}

void machine::configure_memory_map(const memory_map_config& config) {
    impl_->mem.configure(config);
    impl_->ports.clear_bank_port();
}

void machine::configure_banked_memory(const banked_memory_config& config) {
    impl_->mem.configure_compat(config);
    if (config.banked_pages.empty()) {
        impl_->ports.clear_bank_port();
    }
}

void machine::bind_bank_port(uint16_t port) noexcept {
    impl_->ports.bind_bank_port(port);
}

void machine::clear_bank_port() noexcept {
    impl_->ports.clear_bank_port();
}

void machine::set_active_bank(uint16_t bank) noexcept {
    impl_->mem.set_compat_active_bank(bank);
}

uint16_t machine::active_bank() const noexcept {
    return impl_->mem.compat_active_bank();
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

void machine::bind_emu_stdio(std::istream& input, std::ostream& output) noexcept {
    bind_stdin_status_data(emu_stdin_status_port, emu_stdin_data_port, input);
    bind_stdout(emu_stdout_port, output);
}

void machine::bind_host_filesystem(const std::filesystem::path& root) {
    impl_->host_fs_root_ = std::filesystem::absolute(root);
    std::filesystem::create_directories(*impl_->host_fs_root_);
    impl_->files.clear();
}

void machine::clear_stdin() noexcept {
    impl_->ports.clear_stdin();
}

void machine::clear_stdout() noexcept {
    impl_->ports.clear_stdout();
}

void machine::clear_host_filesystem() noexcept {
    impl_->host_fs_root_.reset();
    impl_->files.clear();
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

void remote_session::load_ihx(const std::filesystem::path& path) {
    for (const auto& chunk : read_ihx_chunks(path))
        write_memory(chunk.address,
                     std::span<const uint8_t>(chunk.bytes.data(), chunk.bytes.size()));
}

uint16_t remote_session::load_elf(const std::filesystem::path& path) {
    return load_elf_sections(
        path,
        [this](uint16_t address, std::span<const uint8_t> bytes) {
            write_memory(address, bytes);
        });
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
