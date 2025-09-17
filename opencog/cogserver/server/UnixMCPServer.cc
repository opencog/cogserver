/*
 * opencog/cogserver/server/UnixMCPServer.cc
 *
 * Copyright (C) 2025 OpenCog Foundation
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifdef HAVE_MCP

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <sstream>

#include <opencog/util/Logger.h>
#include <opencog/util/exceptions.h>

#include <opencog/cogserver/server/UnixMCPServer.h>
#include <opencog/cogserver/server/CogServer.h>
#include <opencog/cogserver/shell/McpEval.h>

using namespace opencog;

UnixMCPServer::UnixMCPServer(CogServer& cs, const std::string& socket_path) :
    _socket_path(socket_path),
    _listen_fd(-1),
    _running(false),
    _listener_thread(nullptr),
    _cogserver(cs)
{
    logger().info("UnixMCPServer: Initializing with socket path: %s", _socket_path.c_str());
}

UnixMCPServer::~UnixMCPServer()
{
    stop();
}

void UnixMCPServer::start()
{
    if (_running) return;

    // Create parent directory if it doesn't exist
    size_t last_slash = _socket_path.rfind('/');
    if (last_slash != std::string::npos)
    {
        std::string dir = _socket_path.substr(0, last_slash);
        mkdir(dir.c_str(), 0755);  // Ignore errors, will fail below if really a problem
    }

    // Remove any existing socket file
    unlink(_socket_path.c_str());

    // Create Unix domain socket
    _listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (_listen_fd < 0)
    {
        logger().error("UnixMCPServer: Failed to create socket: %s", strerror(errno));
        throw RuntimeException(TRACE_INFO,
            "Failed to create Unix domain socket: %s", strerror(errno));
    }

    // Set socket to non-blocking for accept
    int flags = fcntl(_listen_fd, F_GETFL, 0);
    fcntl(_listen_fd, F_SETFL, flags | O_NONBLOCK);

    // Bind to socket path
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, _socket_path.c_str(), sizeof(addr.sun_path) - 1);

    if (bind(_listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        close(_listen_fd);
        _listen_fd = -1;
        logger().error("UnixMCPServer: Failed to bind to %s: %s",
                       _socket_path.c_str(), strerror(errno));
        throw RuntimeException(TRACE_INFO,
            "Failed to bind Unix domain socket to %s: %s",
            _socket_path.c_str(), strerror(errno));
    }

    // Set permissions to be readable/writable by all
    chmod(_socket_path.c_str(), 0666);

    // Start listening
    if (listen(_listen_fd, 5) < 0)
    {
        close(_listen_fd);
        _listen_fd = -1;
        unlink(_socket_path.c_str());
        logger().error("UnixMCPServer: Failed to listen on socket: %s", strerror(errno));
        throw RuntimeException(TRACE_INFO,
            "Failed to listen on Unix domain socket: %s", strerror(errno));
    }

    _running = true;
    _listener_thread = new std::thread(&UnixMCPServer::listen_loop, this);

    logger().info("UnixMCPServer: Listening on %s", _socket_path.c_str());
    printf("MCP Unix socket server listening on %s\n", _socket_path.c_str());
}

void UnixMCPServer::stop()
{
    if (!_running) return;

    _running = false;

    // Close the listening socket to interrupt accept()
    if (_listen_fd >= 0)
    {
        close(_listen_fd);
        _listen_fd = -1;
    }

    // Wait for listener thread to finish
    if (_listener_thread)
    {
        _listener_thread->join();
        delete _listener_thread;
        _listener_thread = nullptr;
    }

    // Remove socket file
    unlink(_socket_path.c_str());

    logger().info("UnixMCPServer: Stopped and removed socket at %s", _socket_path.c_str());
}

void UnixMCPServer::listen_loop()
{
    while (_running)
    {
        struct sockaddr_un client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(_listen_fd,
                               (struct sockaddr*)&client_addr,
                               &client_len);

        if (client_fd < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // Non-blocking accept, no connection available
                usleep(100000);  // Sleep 100ms
                continue;
            }
            else if (_running)
            {
                logger().error("UnixMCPServer: accept() failed: %s", strerror(errno));
            }
            break;
        }

        logger().info("UnixMCPServer: Client connected");

        // Handle each client in a separate thread
        std::thread(&UnixMCPServer::handle_client, this, client_fd).detach();
    }
}

void UnixMCPServer::handle_client(int client_fd)
{
    // Create an McpEval instance for this client
    McpEval* eval = McpEval::get_evaluator(_cogserver.getAtomSpace());

    // Buffer for reading data
    char buffer[4096];
    std::string line_buffer;

    while (_running)
    {
        ssize_t n = read(client_fd, buffer, sizeof(buffer) - 1);

        if (n <= 0)
        {
            if (n < 0)
                logger().error("UnixMCPServer: read() failed: %s", strerror(errno));
            else
                logger().info("UnixMCPServer: Client disconnected");
            break;
        }

        buffer[n] = '\0';
        line_buffer += buffer;

        // Process complete lines (JSON objects are typically one per line in MCP)
        size_t pos;
        while ((pos = line_buffer.find('\n')) != std::string::npos)
        {
            std::string line = line_buffer.substr(0, pos);
            line_buffer = line_buffer.substr(pos + 1);

            // Skip empty lines
            if (line.empty() || (line.size() == 1 && line[0] == '\r'))
                continue;

            // Remove trailing \r if present
            if (!line.empty() && line.back() == '\r')
                line.pop_back();

            // Process the JSON-RPC request
            eval->begin_eval();
            eval->eval_expr(line);
            std::string result = eval->poll_result();

            // Send response back to client
            if (!result.empty())
            {
                // Add newline for JSON-RPC framing
                result += "\n";
                ssize_t written = write(client_fd, result.c_str(), result.length());
                if (written < 0)
                {
                    logger().error("UnixMCPServer: write() failed: %s", strerror(errno));
                    break;
                }
            }
        }
    }

    close(client_fd);
}

#endif // HAVE_MCP