/*
 * GDB Remote Serial Protocol (RSP) client and server.
 *
 * The client connects to any gdbserver-compatible target.
 * The server exposes a rsp::target implementation as a gdbserver,
 * allowing any GDB-compatible debugger to connect.
 *
 * Wire format: $packet-data#XX  where XX = sum-of-bytes mod 256 as hex.
 * ACK: '+', NAK: '-', interrupt: 0x03.
 *
 * Z80 register layout in g/G packets (18 bytes, each 16-bit reg LE):
 *   offset  0: AF   offset  2: BC   offset  4: DE   offset  6: HL
 *   offset  8: IX   offset 10: IY   offset 12: SP   offset 14: PC
 *   offset 16: I (1 byte)  offset 17: R (1 byte)
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */

#ifndef RSP_RSP_H
#define RSP_RSP_H

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace rsp {

    using socket_handle = std::intptr_t;
    inline constexpr socket_handle invalid_socket_handle = -1;

    // -----------------------------------------------------------------------
    // Error type
    // -----------------------------------------------------------------------

    class error : public std::runtime_error {
    public:
        using std::runtime_error::runtime_error;
    };

    // -----------------------------------------------------------------------
    // Stop reply returned by client operations
    // -----------------------------------------------------------------------

    struct stop_reply {
        enum class kind { signal, exited, terminated };
        kind    type          = kind::signal;
        uint8_t signal_number = 5;   // 5 = SIGTRAP
        int     exit_code     = 0;
    };

    // -----------------------------------------------------------------------
    // Abstract target interface implemented by the gdbserver side
    // -----------------------------------------------------------------------

    class target {
    public:
        virtual ~target() = default;

        // Return raw register bytes (18 bytes for Z80 in the layout above).
        virtual std::vector<uint8_t> read_registers() = 0;

        // Write raw register bytes.
        virtual void write_registers(const std::vector<uint8_t>& data) = 0;

        // Read len bytes from target memory starting at addr.
        virtual std::vector<uint8_t> read_memory(
            uint32_t addr, std::size_t len) = 0;

        // Write bytes into target memory starting at addr.
        virtual void write_memory(
            uint32_t addr, const std::vector<uint8_t>& data) = 0;

        // Resume execution; block until the target stops.
        // Returns an RSP stop reply string, e.g. "S05" or "W00".
        virtual std::string cont() = 0;

        // Execute one instruction.
        // Returns an RSP stop reply string, e.g. "S05".
        virtual std::string step() = 0;

        // Return the current stop reason (used to answer '?').
        // Returns an RSP stop reply string, e.g. "S05".
        virtual std::string stop_reason() = 0;

        // Install a software breakpoint at addr.
        virtual void insert_breakpoint(uint32_t addr) = 0;

        // Remove the breakpoint at addr.
        virtual void remove_breakpoint(uint32_t addr) = 0;

        // Detach the debugger; target continues running.
        virtual void detach() = 0;
    };

    // -----------------------------------------------------------------------
    // Server — exposes a rsp::target as a gdbserver over TCP
    // -----------------------------------------------------------------------

    class server {
    public:
        server() = default;
        ~server();
        server(const server&) = delete;
        server& operator=(const server&) = delete;
        server(server&&) noexcept;
        server& operator=(server&&) noexcept;

        // Open the listening socket.
        void listen(const std::string& host, uint16_t port);

        // Close the listening socket and any active client connection.
        void close();

        // True when the listening socket is open.
        bool is_listening() const;

        // Accept one client connection and serve it until it disconnects.
        // Blocks until the client detaches or the connection is lost.
        void serve(target& tgt);

    private:
        std::atomic<socket_handle> listen_fd_      {invalid_socket_handle};
        std::atomic<socket_handle> client_fd_      {invalid_socket_handle};
        std::atomic<bool> stop_requested_ {false};
    };

    // -----------------------------------------------------------------------
    // Client — connects to a gdbserver-compatible target
    // -----------------------------------------------------------------------

    class client {
    public:
        client() = default;
        ~client();
        client(const client&) = delete;
        client& operator=(const client&) = delete;
        client(client&&) noexcept;
        client& operator=(client&&) noexcept;

        // Connect to host:port.
        void connect(const std::string& host, uint16_t port);

        // Close the connection.
        void close();

        // True when connected.
        bool is_connected() const;

        // Send '?' and return the current stop state.
        stop_reply query_stop();

        // Send 'g' and return the raw register bytes.
        std::vector<uint8_t> read_registers();

        // Send 'G' with the raw register bytes.
        void write_registers(const std::vector<uint8_t>& data);

        // Send 'm addr,len' and return the bytes read.
        std::vector<uint8_t> read_memory(uint32_t addr, std::size_t len);

        // Send 'M addr,len:hexdata'.
        void write_memory(uint32_t addr, const std::vector<uint8_t>& data);

        // Send 'c' and block until the target stops.
        stop_reply cont();

        // Send 's' and block until the step completes.
        stop_reply step();

        // Send interrupt byte 0x03.
        void pause();

        // Send 'Z0,addr,1' to install a software breakpoint.
        void insert_breakpoint(uint32_t addr);

        // Send 'z0,addr,1' to remove a software breakpoint.
        void remove_breakpoint(uint32_t addr);

        // Send 'D' to detach.
        void detach();

    private:
        // Send a packet and return the response data.
        std::string transact(const std::string& data);

        // Receive one framed RSP packet; returns the data inside $...#.
        std::string recv_packet();

        // Parse a stop reply string like "S05", "W00", "X0b".
        static stop_reply parse_stop_reply(const std::string& reply);

        socket_handle fd_ = invalid_socket_handle;
    };

} // namespace rsp

#endif // RSP_RSP_H
