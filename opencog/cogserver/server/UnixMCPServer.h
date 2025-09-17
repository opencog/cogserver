/*
 * opencog/cogserver/server/UnixMCPServer.h
 *
 * Copyright (C) 2025 OpenCog Foundation
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef _OPENCOG_UNIX_MCP_SERVER_H
#define _OPENCOG_UNIX_MCP_SERVER_H

#include <string>
#include <thread>
#include <atomic>

namespace opencog
{

class CogServer;
class McpEval;

/** \addtogroup grp_server
 *  @{
 */

/**
 * This class implements a Unix domain socket server for the MCP protocol.
 * It listens on a Unix socket (e.g., /run/cogserver/mcp or /tmp/cogserver/mcp)
 * and handles MCP JSON-RPC requests directly.
 */
class UnixMCPServer
{
private:
    std::string _socket_path;
    int _listen_fd;
    std::atomic_bool _running;
    std::thread* _listener_thread;
    CogServer& _cogserver;

    void listen_loop();
    void handle_client(int client_fd);

public:
    UnixMCPServer(CogServer& cs, const std::string& socket_path);
    ~UnixMCPServer();

    void start();
    void stop();

    const std::string& getSocketPath() const { return _socket_path; }
    bool isRunning() const { return _running; }
}; // class

/** @}*/
}  // namespace

#endif // _OPENCOG_UNIX_MCP_SERVER_H