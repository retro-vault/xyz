/*
 * GDB Remote Serial Protocol implementation.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <rsp/rsp.h>

namespace rsp {

namespace {

// ---------------------------------------------------------------------------
// Low-level socket helpers
// ---------------------------------------------------------------------------

void throw_errno(const char* prefix) {
    throw error(std::string(prefix) + ": " + std::strerror(errno));
}

void fd_close(int& fd) {
    if (fd >= 0) { ::close(fd); fd = -1; }
}

void fd_shutdown_close(int fd) {
    if (fd >= 0) { ::shutdown(fd, SHUT_RDWR); ::close(fd); }
}

void atomic_close(std::atomic<int>& afd) {
    fd_shutdown_close(afd.exchange(-1));
}

void send_all(int fd, const void* buf, std::size_t len) {
    const auto* p = static_cast<const char*>(buf);
    while (len > 0) {
        ssize_t n = ::send(fd, p, len, 0);
        if (n < 0) throw_errno("send");
        if (n == 0) throw error("connection closed during send");
        p += n; len -= static_cast<std::size_t>(n);
    }
}

void send_str(int fd, const std::string& s) {
    send_all(fd, s.data(), s.size());
}

int recv_byte(int fd) {
    uint8_t ch;
    ssize_t n = ::recv(fd, &ch, 1, 0);
    if (n < 0) throw_errno("recv");
    if (n == 0) return -1;   // EOF
    return static_cast<int>(ch);
}

int connect_socket(const std::string& host, uint16_t port) {
    addrinfo hints{};
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* info = nullptr;
    const std::string ps = std::to_string(port);
    if (::getaddrinfo(host.c_str(), ps.c_str(), &hints, &info) != 0)
        throw error("getaddrinfo failed for " + host);

    int fd = -1;
    for (auto* c = info; c; c = c->ai_next) {
        fd = ::socket(c->ai_family, c->ai_socktype, c->ai_protocol);
        if (fd < 0) continue;
        if (::connect(fd, c->ai_addr, c->ai_addrlen) == 0) break;
        ::close(fd); fd = -1;
    }
    ::freeaddrinfo(info);
    if (fd < 0) throw error("cannot connect to " + host + ":" + ps);
    return fd;
}

int listen_socket(const std::string& host, uint16_t port) {
    addrinfo hints{};
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = AI_PASSIVE;

    addrinfo* info = nullptr;
    const std::string ps = std::to_string(port);
    const char* hp = host.empty() ? nullptr : host.c_str();
    if (::getaddrinfo(hp, ps.c_str(), &hints, &info) != 0)
        throw error("getaddrinfo failed");

    int fd = -1;
    for (auto* c = info; c; c = c->ai_next) {
        fd = ::socket(c->ai_family, c->ai_socktype, c->ai_protocol);
        if (fd < 0) continue;
        int one = 1;
        ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
        if (::bind(fd, c->ai_addr, c->ai_addrlen) == 0 &&
            ::listen(fd, 1) == 0) break;
        ::close(fd); fd = -1;
    }
    ::freeaddrinfo(info);
    if (fd < 0) throw error("cannot bind to port " + ps);
    return fd;
}

// ---------------------------------------------------------------------------
// RSP packet framing
// ---------------------------------------------------------------------------

static const char HEX[] = "0123456789abcdef";

char hex_nibble(uint8_t v) { return HEX[v & 0x0F]; }

uint8_t checksum(const std::string& data) {
    uint8_t sum = 0;
    for (char c : data) sum += static_cast<uint8_t>(c);
    return sum;
}

std::string encode_packet(const std::string& data) {
    uint8_t cs = checksum(data);
    std::string pkt;
    pkt.reserve(data.size() + 4);
    pkt += '$';
    pkt += data;
    pkt += '#';
    pkt += hex_nibble(cs >> 4);
    pkt += hex_nibble(cs);
    return pkt;
}

// Receive one complete $data#XX packet from fd.
// Handles leading '+'/'-' and interrupt bytes, skips garbage.
std::string recv_rsp_packet(int fd) {
    while (true) {
        int ch = recv_byte(fd);
        if (ch < 0) throw error("connection closed");
        if (ch == '+' || ch == '-') continue;  // ACK/NAK from previous send
        if (ch == 0x03) continue;               // stray interrupt
        if (ch != '$') continue;                // skip garbage

        // read data until '#'
        std::string data;
        while (true) {
            int b = recv_byte(fd);
            if (b < 0) throw error("connection closed in packet body");
            if (b == '#') break;
            if (b == 0x7D) {
                // escape byte: next byte XOR 0x20
                int esc = recv_byte(fd);
                if (esc < 0) throw error("connection closed after escape");
                data += static_cast<char>(esc ^ 0x20);
            } else {
                data += static_cast<char>(b);
            }
        }

        // read 2 checksum chars (ignore for now, trust the connection)
        recv_byte(fd);
        recv_byte(fd);

        // send ACK
        send_all(fd, "+", 1);
        return data;
    }
}

// ---------------------------------------------------------------------------
// Hex encode/decode utilities
// ---------------------------------------------------------------------------

std::string bytes_to_hex(const std::vector<uint8_t>& v) {
    std::string s;
    s.reserve(v.size() * 2);
    for (uint8_t b : v) {
        s += hex_nibble(b >> 4);
        s += hex_nibble(b);
    }
    return s;
}

std::vector<uint8_t> hex_to_bytes(const std::string& s) {
    std::vector<uint8_t> v;
    v.reserve(s.size() / 2);
    for (std::size_t i = 0; i + 1 < s.size(); i += 2) {
        auto from_hex = [](char c) -> uint8_t {
            if (c >= '0' && c <= '9') return static_cast<uint8_t>(c - '0');
            if (c >= 'a' && c <= 'f') return static_cast<uint8_t>(c - 'a' + 10);
            if (c >= 'A' && c <= 'F') return static_cast<uint8_t>(c - 'A' + 10);
            return 0;
        };
        v.push_back(static_cast<uint8_t>(
            (from_hex(s[i]) << 4) | from_hex(s[i + 1])));
    }
    return v;
}

uint32_t parse_hex_addr(const std::string& s) {
    return static_cast<uint32_t>(std::strtoul(s.c_str(), nullptr, 16));
}

std::string hex_addr(uint32_t addr) {
    char buf[9];
    std::snprintf(buf, sizeof(buf), "%x", addr);
    return buf;
}

// ---------------------------------------------------------------------------
// Stop reply parsing
// ---------------------------------------------------------------------------

stop_reply parse_stop(const std::string& r) {
    stop_reply sr;
    if (r.empty()) { sr.type = stop_reply::kind::signal; sr.signal_number = 5; return sr; }

    if (r[0] == 'S' || r[0] == 'T') {
        sr.type = stop_reply::kind::signal;
        if (r.size() >= 3) {
            sr.signal_number = static_cast<uint8_t>(
                std::strtoul(r.substr(1, 2).c_str(), nullptr, 16));
        }
    } else if (r[0] == 'W') {
        sr.type = stop_reply::kind::exited;
        if (r.size() >= 3) {
            sr.exit_code = static_cast<int>(
                std::strtoul(r.substr(1, 2).c_str(), nullptr, 16));
        }
    } else if (r[0] == 'X') {
        sr.type = stop_reply::kind::terminated;
        if (r.size() >= 3) {
            sr.signal_number = static_cast<uint8_t>(
                std::strtoul(r.substr(1, 2).c_str(), nullptr, 16));
        }
    }
    return sr;
}

// ---------------------------------------------------------------------------
// Server packet dispatcher
// ---------------------------------------------------------------------------

void serve_client(int client_fd, target& tgt, const std::atomic<bool>& stop_req) {
    while (!stop_req.load()) {
        // Peek at the next byte to handle interrupt vs packet.
        int first;
        do {
            first = recv_byte(client_fd);
            if (first < 0) return;  // EOF
        } while (first == '+' || first == '-');

        if (first == 0x03) {
            // Ctrl-C interrupt — target already stopped (sync model).
            // Just send the current stop reason.
            const std::string reply = tgt.stop_reason();
            send_str(client_fd, encode_packet(reply));
            continue;
        }

        if (first != '$') continue;  // skip garbage

        // Read packet body until '#'.
        std::string data;
        while (true) {
            int b = recv_byte(client_fd);
            if (b < 0) return;
            if (b == '#') break;
            if (b == 0x7D) {
                int esc = recv_byte(client_fd);
                if (esc < 0) return;
                data += static_cast<char>(esc ^ 0x20);
            } else {
                data += static_cast<char>(b);
            }
        }
        recv_byte(client_fd);  // checksum hi
        recv_byte(client_fd);  // checksum lo
        send_all(client_fd, "+", 1);

        // Dispatch.
        std::string response;

        if (data == "?") {
            response = tgt.stop_reason();

        } else if (data == "g") {
            response = bytes_to_hex(tgt.read_registers());

        } else if (!data.empty() && data[0] == 'G') {
            tgt.write_registers(hex_to_bytes(data.substr(1)));
            response = "OK";

        } else if (!data.empty() && data[0] == 'm') {
            const auto comma = data.find(',');
            if (comma != std::string::npos) {
                uint32_t addr = parse_hex_addr(data.substr(1, comma - 1));
                std::size_t len = static_cast<std::size_t>(
                    parse_hex_addr(data.substr(comma + 1)));
                response = bytes_to_hex(tgt.read_memory(addr, len));
            }

        } else if (!data.empty() && data[0] == 'M') {
            const auto comma = data.find(',');
            const auto colon = data.find(':');
            if (comma != std::string::npos && colon != std::string::npos) {
                uint32_t addr = parse_hex_addr(data.substr(1, comma - 1));
                tgt.write_memory(addr, hex_to_bytes(data.substr(colon + 1)));
                response = "OK";
            }

        } else if (data == "c" || (!data.empty() && data[0] == 'c')) {
            response = tgt.cont();

        } else if (data == "s" || (!data.empty() && data[0] == 's')) {
            response = tgt.step();

        } else if (data.size() >= 2 && data[0] == 'Z' && data[1] == '0') {
            const auto comma = data.find(',');
            if (comma != std::string::npos) {
                uint32_t addr = parse_hex_addr(data.substr(3, comma - 3));
                tgt.insert_breakpoint(addr);
                response = "OK";
            }

        } else if (data.size() >= 2 && data[0] == 'z' && data[1] == '0') {
            const auto comma = data.find(',');
            if (comma != std::string::npos) {
                uint32_t addr = parse_hex_addr(data.substr(3, comma - 3));
                tgt.remove_breakpoint(addr);
                response = "OK";
            }

        } else if (data == "D") {
            tgt.detach();
            send_str(client_fd, encode_packet("OK"));
            break;

        } else if (data == "k") {
            break;

        } else if (data.rfind("qSupported", 0) == 0) {
            response = "PacketSize=4000;swbreak+";

        } else if (data.rfind("qAttached", 0) == 0) {
            response = "1";

        } else if (!data.empty() && data[0] == 'H') {
            response = "OK";

        } else if (data == "vMustReplyEmpty" || data.rfind("vCont", 0) == 0) {
            response = "";

        } else if (data == "qfThreadInfo") {
            response = "m1";

        } else if (data == "qsThreadInfo") {
            response = "l";

        } else if (data.rfind("qThreadExtraInfo", 0) == 0) {
            response = bytes_to_hex(
                std::vector<uint8_t>{'Z','8','0',' ','t','h','r','e','a','d'});

        } else {
            response = "";  // unsupported — empty means "not supported"
        }

        send_str(client_fd, encode_packet(response));
    }
}

} // namespace

// ---------------------------------------------------------------------------
// server
// ---------------------------------------------------------------------------

server::~server() { close(); }

server::server(server&& o) noexcept
    : listen_fd_(o.listen_fd_.exchange(-1))
    , client_fd_(o.client_fd_.exchange(-1))
    , stop_requested_(o.stop_requested_.load())
{ o.stop_requested_ = false; }

server& server::operator=(server&& o) noexcept {
    if (this != &o) {
        close();
        listen_fd_      = o.listen_fd_.exchange(-1);
        client_fd_      = o.client_fd_.exchange(-1);
        stop_requested_ = o.stop_requested_.load();
        o.stop_requested_ = false;
    }
    return *this;
}

void server::listen(const std::string& host, uint16_t port) {
    close();
    stop_requested_ = false;
    listen_fd_ = listen_socket(host, port);
}

void server::close() {
    stop_requested_ = true;
    atomic_close(client_fd_);
    atomic_close(listen_fd_);
}

bool server::is_listening() const { return listen_fd_.load() >= 0; }

void server::serve(target& tgt) {
    const int lfd = listen_fd_.load();
    if (lfd < 0) throw error("server is not listening");

    sockaddr_storage addr{};
    socklen_t len = sizeof(addr);
    int cfd = ::accept(lfd, reinterpret_cast<sockaddr*>(&addr), &len);
    if (cfd < 0) {
        if (stop_requested_.load()) return;
        throw_errno("accept");
    }
    client_fd_ = cfd;

    try {
        serve_client(cfd, tgt, stop_requested_);
    } catch (...) {
        int expected = cfd;
        if (client_fd_.compare_exchange_strong(expected, -1))
            fd_shutdown_close(cfd);
        if (!stop_requested_.load()) throw;
        return;
    }

    int expected = cfd;
    if (client_fd_.compare_exchange_strong(expected, -1))
        fd_shutdown_close(cfd);
}

// ---------------------------------------------------------------------------
// client
// ---------------------------------------------------------------------------

client::~client() { close(); }

client::client(client&& o) noexcept : fd_(o.fd_) { o.fd_ = -1; }

client& client::operator=(client&& o) noexcept {
    if (this != &o) { close(); fd_ = o.fd_; o.fd_ = -1; }
    return *this;
}

void client::connect(const std::string& host, uint16_t port) {
    close();
    fd_ = connect_socket(host, port);
}

void client::close() { fd_close(fd_); }

bool client::is_connected() const { return fd_ >= 0; }

std::string client::recv_packet() {
    return recv_rsp_packet(fd_);
}

std::string client::transact(const std::string& data) {
    send_str(fd_, encode_packet(data));
    // consume ACK from server
    int ack;
    do { ack = recv_byte(fd_); } while (ack == '+' || ack == '-');
    // put ack back... can't easily unread, but for RSP the ACK isn't
    // part of the response. If ack == '$', fall through to recv_packet.
    // We need to reconstruct the packet manually here.
    if (ack < 0) throw error("connection closed waiting for response");

    // Re-assemble: we already consumed the first byte.
    if (ack != '$') {
        // Unexpected — skip and read a full packet.
        return recv_rsp_packet(fd_);
    }

    // Read body until '#'.
    std::string result;
    while (true) {
        int b = recv_byte(fd_);
        if (b < 0) throw error("connection closed in response");
        if (b == '#') break;
        if (b == 0x7D) {
            int esc = recv_byte(fd_);
            if (esc < 0) throw error("connection closed after escape");
            result += static_cast<char>(esc ^ 0x20);
        } else {
            result += static_cast<char>(b);
        }
    }
    recv_byte(fd_);  // checksum hi
    recv_byte(fd_);  // checksum lo
    send_all(fd_, "+", 1);
    return result;
}

stop_reply client::parse_stop_reply(const std::string& r) {
    return parse_stop(r);
}

stop_reply client::query_stop() {
    return parse_stop_reply(transact("?"));
}

std::vector<uint8_t> client::read_registers() {
    return hex_to_bytes(transact("g"));
}

void client::write_registers(const std::vector<uint8_t>& data) {
    const std::string resp = transact("G" + bytes_to_hex(data));
    if (resp != "OK") throw error("write_registers failed: " + resp);
}

std::vector<uint8_t> client::read_memory(uint32_t addr, std::size_t len) {
    const std::string cmd = "m" + hex_addr(addr) + "," + hex_addr(len);
    const std::string resp = transact(cmd);
    if (!resp.empty() && resp[0] == 'E')
        throw error("read_memory error: " + resp);
    return hex_to_bytes(resp);
}

void client::write_memory(uint32_t addr, const std::vector<uint8_t>& data) {
    const std::string cmd = "M" + hex_addr(addr) + ","
                          + hex_addr(data.size()) + ":"
                          + bytes_to_hex(data);
    const std::string resp = transact(cmd);
    if (resp != "OK") throw error("write_memory failed: " + resp);
}

stop_reply client::cont() {
    // 'c' is special: server ACKs then sends stop reply when target halts.
    send_str(fd_, encode_packet("c"));
    // Consume server ACK for our packet.
    int b;
    do { b = recv_byte(fd_); } while (b == '+' || b == '-');
    // Now b should be '$' of the stop reply packet.
    if (b != '$') {
        // Unexpected, try to re-sync.
        return parse_stop_reply(recv_rsp_packet(fd_));
    }
    std::string reply;
    while (true) {
        int x = recv_byte(fd_);
        if (x < 0) throw error("connection closed waiting for stop reply");
        if (x == '#') break;
        reply += static_cast<char>(x);
    }
    recv_byte(fd_); recv_byte(fd_);   // checksum
    send_all(fd_, "+", 1);
    return parse_stop_reply(reply);
}

stop_reply client::step() {
    send_str(fd_, encode_packet("s"));
    int b;
    do { b = recv_byte(fd_); } while (b == '+' || b == '-');
    if (b != '$') return parse_stop_reply(recv_rsp_packet(fd_));
    std::string reply;
    while (true) {
        int x = recv_byte(fd_);
        if (x < 0) throw error("connection closed waiting for stop reply");
        if (x == '#') break;
        reply += static_cast<char>(x);
    }
    recv_byte(fd_); recv_byte(fd_);
    send_all(fd_, "+", 1);
    return parse_stop_reply(reply);
}

void client::pause() {
    const char intr = 0x03;
    send_all(fd_, &intr, 1);
}

void client::insert_breakpoint(uint32_t addr) {
    const std::string cmd = "Z0," + hex_addr(addr) + ",1";
    const std::string resp = transact(cmd);
    if (resp != "OK" && !resp.empty())
        throw error("insert_breakpoint failed: " + resp);
}

void client::remove_breakpoint(uint32_t addr) {
    const std::string cmd = "z0," + hex_addr(addr) + ",1";
    const std::string resp = transact(cmd);
    if (resp != "OK" && !resp.empty())
        throw error("remove_breakpoint failed: " + resp);
}

void client::detach() {
    try { transact("D"); } catch (...) {}
}

} // namespace rsp
