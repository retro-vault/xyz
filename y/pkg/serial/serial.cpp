// serial.cpp — ZX Spectrum serial file transfer tool.
//
// Usage:
//   serial put <device> <file> <addr>   send file to ZX Spectrum
//   serial get <device> <file>          receive file from ZX Spectrum

#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// ZX Spectrum serial header — sent by the PC to the Spectrum.
// type=3 means CODE block; par1=load address, par2=0xFFFF, reserved=0.
// Packed to avoid any compiler padding between fields.
// ---------------------------------------------------------------------------
#pragma pack(push, 1)
struct tx_header {
    uint8_t  type;      // block type: 3 = CODE
    uint16_t data_len;  // payload length in bytes
    uint16_t par1;      // load address
    uint16_t par2;      // 0xFFFF for CODE blocks
    uint16_t reserved;  // always 0 (protocol requirement)
};
static_assert(sizeof(tx_header) == 9);

// ---------------------------------------------------------------------------
// ZX Spectrum serial header — received by the PC from the Spectrum.
// Same fields as tx_header but without the trailing reserved word.
// ---------------------------------------------------------------------------
struct rx_header {
    uint8_t  type;      // block type
    uint16_t data_len;  // payload length in bytes
    uint16_t par1;      // parameter 1
    uint16_t par2;      // parameter 2
};
static_assert(sizeof(rx_header) == 7);
#pragma pack(pop)

// ---------------------------------------------------------------------------
// RAII wrapper for a serial port file descriptor.
// ---------------------------------------------------------------------------
struct serial_port {
    int fd { -1 };

    serial_port() = default;
    explicit serial_port(int fd) : fd(fd) {}
    serial_port(const serial_port&)            = delete;
    serial_port& operator=(const serial_port&) = delete;
    serial_port(serial_port&& o) noexcept : fd(std::exchange(o.fd, -1)) {}
    serial_port& operator=(serial_port&& o) noexcept {
        if (this != &o) { release(); fd = std::exchange(o.fd, -1); }
        return *this;
    }
    ~serial_port() { release(); }

private:
    void release() { if (fd != -1) { ::close(fd); fd = -1; } }
};

// ---------------------------------------------------------------------------
// Open and configure a serial port: 2400 baud, 8N1, hardware RTS/CTS.
// ---------------------------------------------------------------------------
serial_port open_port(std::string_view device) {
    int fd = ::open(device.data(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1)
        throw std::runtime_error(std::format("cannot open device: {}", device));

    ::fcntl(fd, F_SETFL, 0);

    termios opts {};
    ::tcgetattr(fd, &opts);

    ::cfsetispeed(&opts, B2400);
    ::cfsetospeed(&opts, B2400);

    opts.c_cflag &= ~PARENB;   // no parity
    opts.c_cflag &= ~CSTOPB;   // one stop bit
    opts.c_cflag &= ~CSIZE;    // clear char-size mask
    opts.c_cflag |=  CS8;      // 8-bit characters
    opts.c_cflag |=  CRTSCTS;  // hardware RTS/CTS flow control
    opts.c_oflag &= ~OPOST;    // raw output

    ::tcsetattr(fd, TCSANOW, &opts);
    return serial_port { fd };
}

// ---------------------------------------------------------------------------
// Blocking write — retries until all bytes are sent.
// ---------------------------------------------------------------------------
void send_bytes(int fd, std::span<const std::byte> data) {
    const auto* p     = data.data();
    std::size_t left  = data.size();
    while (left) {
        ssize_t n = ::write(fd, p, left);
        if (n < 0) throw std::runtime_error("write error on serial port");
        p    += n;
        left -= static_cast<std::size_t>(n);
    }
}

// ---------------------------------------------------------------------------
// Blocking read — retries until all bytes are received.
// ---------------------------------------------------------------------------
void recv_bytes(int fd, std::span<std::byte> data) {
    auto*       p    = data.data();
    std::size_t left = data.size();
    while (left) {
        ssize_t n = ::read(fd, p, left);
        if (n < 0) throw std::runtime_error("read error on serial port");
        p    += n;
        left -= static_cast<std::size_t>(n);
    }
}

// ---------------------------------------------------------------------------
// Send a binary file to the ZX Spectrum.
// ---------------------------------------------------------------------------
void cmd_put(std::string_view device, const fs::path& file, uint16_t addr) {
    if (!fs::exists(file))
        throw std::runtime_error(std::format("file not found: {}", file.string()));

    std::vector<std::byte> data(fs::file_size(file));
    {
        std::ifstream ifs(file, std::ios::binary);
        if (!ifs) throw std::runtime_error(std::format("cannot read: {}", file.string()));
        ifs.read(reinterpret_cast<char*>(data.data()),
                 static_cast<std::streamsize>(data.size()));
    }

    auto port = open_port(device);

    tx_header hdr {
        .type     = 3,
        .data_len = static_cast<uint16_t>(data.size()),
        .par1     = addr,
        .par2     = 0xFFFF,
        .reserved = 0,
    };

    std::cout << std::format("put: {} -> {} addr=0x{:04X} len={}\n",
        file.string(), device, addr, data.size());

    send_bytes(port.fd, std::as_bytes(std::span { &hdr, 1 }));
    send_bytes(port.fd, std::span { data });

    std::cout << "done.\n";
}

// ---------------------------------------------------------------------------
// Receive a binary file from the ZX Spectrum.
// ---------------------------------------------------------------------------
void cmd_get(std::string_view device, const fs::path& file) {
    auto port = open_port(device);

    rx_header hdr {};
    recv_bytes(port.fd, std::as_writable_bytes(std::span { &hdr, 1 }));

    std::cout << std::format("get: type={} len={} par1=0x{:04X} par2=0x{:04X}\n",
        hdr.type, hdr.data_len, hdr.par1, hdr.par2);

    std::vector<std::byte> data(hdr.data_len);
    recv_bytes(port.fd, std::span { data });

    {
        std::ofstream ofs(file, std::ios::binary);
        if (!ofs) throw std::runtime_error(std::format("cannot write: {}", file.string()));
        ofs.write(reinterpret_cast<const char*>(data.data()),
                  static_cast<std::streamsize>(data.size()));
    }

    std::cout << std::format("saved {} bytes -> {}\n", data.size(), file.string());
}

// ---------------------------------------------------------------------------
// Entry point.
// ---------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "usage:\n"
                  << "  serial put <device> <file> <addr>\n"
                  << "  serial get <device> <file>\n";
        return 1;
    }

    const std::string_view cmd    { argv[1] };
    const std::string_view device { argv[2] };
    const fs::path         file   { argv[3] };

    try {
        if (cmd == "put") {
            if (argc < 5) { std::cerr << "put requires <addr>\n"; return 1; }
            const auto addr = static_cast<uint16_t>(std::stoul(argv[4]));
            cmd_put(device, file, addr);
        } else if (cmd == "get") {
            cmd_get(device, file);
        } else {
            std::cerr << std::format("unknown command: {}\n", cmd);
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << std::format("error: {}\n", e.what());
        return 2;
    }
}
