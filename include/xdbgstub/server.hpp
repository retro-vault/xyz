/*
 * Declares the xdbgstub server used to listen for debugger clients
 * and bridge their requests to a concrete target implementation.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */

#ifndef XDBGSTUB_SERVER_HPP
#define XDBGSTUB_SERVER_HPP

#include <cstdint>
#include <string>

#include <xdbgstub/model.hpp>

namespace xdbgstub {

    // Debugger protocol server for a single hosted target endpoint.
    class server {
    public:
        // Construct a server in the closed state.
        //
        // Parameters:
        //      None.
        //
        // Returns:
        //      New server object.
        server() = default;

        // Close the listening socket and release server resources.
        //
        // Parameters:
        //      None.
        //
        // Returns:
        //      Nothing.
        ~server();

        /*
         * Disable copying of server instances that own sockets.
         */
        server(const server&) = delete;

        /*
         * Disable copy assignment of server instances that own sockets.
         */
        server& operator=(const server&) = delete;

        // Transfer ownership of an existing listening socket.
        //
        // Parameters:
        //      other       - Server instance to move from.
        //
        // Returns:
        //      New server object.
        server(server&& other) noexcept;

        // Replace this server with another server's listening state.
        //
        // Parameters:
        //      other       - Server instance to move from.
        //
        // Returns:
        //      Reference to this server.
        server& operator=(server&& other) noexcept;

        /*
         * Open the listening socket on the requested host and port.
         *
         * Parameters:
         *      bind_host   - Local host or interface to bind.
         *      port        - Local TCP port number.
         */
        void listen(const std::string& bind_host, uint16_t port);

        // Close the listening socket.
        //
        // Parameters:
        //      None.
        //
        // Returns:
        //      Nothing.
        void close();

        // Check whether the server is currently listening.
        //
        // Parameters:
        //      None.
        //
        // Returns:
        //      True when a listening socket is open, otherwise false.
        bool is_listening() const;

        /*
         * Serve debugger requests against the supplied target.
         *
         * Parameters:
         *      debug_target - Target implementation exposed to clients.
         */
        void serve(target& debug_target);

    private:
        int listen_fd_ = -1; // Listening socket file descriptor, or `-1`.
    };

} // namespace xdbgstub

#endif // XDBGSTUB_SERVER_HPP
