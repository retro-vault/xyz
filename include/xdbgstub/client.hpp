/*
 * Declares the xdbgstub client used to connect to a remote target,
 * exchange debugger protocol messages, and control execution from
 * a hosted debugging frontend.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */

#ifndef XDBGSTUB_CLIENT_HPP
#define XDBGSTUB_CLIENT_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <xdbgstub/model.hpp>

namespace xdbgstub {

    // Debugger protocol client for hosted remote-target sessions.
    class client {
    public:
        // Construct a disconnected debugger client.
        //
        // Parameters:
        //      None.
        //
        // Returns:
        //      New client object.
        client() = default;

        // Close the active connection and release client resources.
        //
        // Parameters:
        //      None.
        //
        // Returns:
        //      Nothing.
        ~client();

        // Disable copying of connected client instances.
        client(const client&) = delete;

        // Disable copy assignment of connected client instances.
        client& operator=(const client&) = delete;

        // Transfer ownership of an existing client connection.
        //
        // Parameters:
        //      other       - Client instance to move from.
        //
        // Returns:
        //      New client object.
        client(client&& other) noexcept;

        // Replace this client with another client's connection state.
        //
        // Parameters:
        //      other       - Client instance to move from.
        //
        // Returns:
        //      Reference to this client.
        client& operator=(client&& other) noexcept;

        /*
         * Connect to a remote debugger stub.
         *
         * Parameters:
         *      host        - Remote host name or address.
         *      port        - Remote TCP port number.
         */
        void connect(const std::string& host, uint16_t port);

        // Close the active debugger connection.
        //
        // Parameters:
        //      None.
        //
        // Returns:
        //      Nothing.
        void close();

        // Check whether the client currently has an open connection.
        //
        // Parameters:
        //      None.
        //
        // Returns:
        //      True when connected, otherwise false.
        bool is_connected() const;

        // Verify that the remote stub is responsive.
        //
        // Parameters:
        //      None.
        //
        // Returns:
        //      Nothing.
        void ping();

        // Query the current execution state of the target.
        //
        // Parameters:
        //      None.
        //
        // Returns:
        //      Current target status snapshot.
        target_status status();

        // Read the full CPU register set from the target.
        //
        // Parameters:
        //      None.
        //
        // Returns:
        //      Register snapshot returned by the remote target.
        cpu_state read_registers();

        /*
         * Replace the target CPU register set.
         *
         * Parameters:
         *      state       - Register values to write.
         */
        void write_registers(const cpu_state& state);

        /*
         * Read a block of target memory.
         *
         * Parameters:
         *      address     - Start address to read from.
         *      length      - Number of bytes to read.
         *
         * Returns:
         *      Bytes read from target memory.
         */
        std::vector<uint8_t> read_memory(uint32_t address, std::size_t length);

        /*
         * Write a block of bytes into target memory.
         *
         * Parameters:
         *      address     - Start address to write to.
         *      data        - Bytes to store at the target address.
         */
        void write_memory(uint32_t address, const std::vector<uint8_t>& data);

        // Resume target execution until the next stop event.
        //
        // Parameters:
        //      None.
        //
        // Returns:
        //      Target status after execution stops or terminates.
        target_status continue_execution();

        // Execute exactly one target instruction when possible.
        //
        // Parameters:
        //      None.
        //
        // Returns:
        //      Target status after the single-step operation.
        target_status step_instruction();

        // Request that the running target pause execution.
        //
        // Parameters:
        //      None.
        //
        // Returns:
        //      Target status after the pause request completes.
        target_status pause_execution();

        /*
         * Install a breakpoint at the specified address.
         *
         * Parameters:
         *      address     - Target address to break on.
         */
        void set_breakpoint(uint32_t address);

        /*
         * Remove a breakpoint at the specified address.
         *
         * Parameters:
         *      address     - Target address to clear.
         */
        void clear_breakpoint(uint32_t address);

        // Detach from the target without issuing further requests.
        //
        // Parameters:
        //      None.
        //
        // Returns:
        //      Nothing.
        void detach();

    private:
        int socket_fd_ = -1;             // Connected socket file descriptor, or `-1`.
        uint32_t next_request_id_ = 1;   // Next protocol request identifier.
    };

} // namespace xdbgstub

#endif // XDBGSTUB_CLIENT_HPP
