/*
 * opencog/cogserver/server/CogServer.cc
 *
 * Copyright (C) 2002-2007 Novamente LLC
 * Copyright (C) 2008 by OpenCog Foundation
 * Written by Andre Senna <senna@vettalabs.com>
 *            Gustavo Gama <gama@vettalabs.com>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/prctl.h>

#include <opencog/util/Logger.h>
#include <opencog/util/misc.h>
#include <opencog/util/platform.h>

#include <opencog/network/NetworkServer.h>

#include <opencog/atoms/core/NumberNode.h>
#include <opencog/atoms/value/FloatValue.h>
#include <opencog/cogserver/atoms/CogServerNode.h>
#include <opencog/cogserver/server/ServerConsole.h>
#include <opencog/cogserver/server/WebServer.h>
#include <opencog/cogserver/server/MCPServer.h>

#include "CogServer.h"

using namespace opencog;

// Helper to extract port value from a CogServerNode.
// Accepts FloatValue or NumberNode.
// Returns zero if key not found or value not understood.
static int get_port(const Handle& hcsn, const char* key)
{
    AtomSpace* asp = hcsn->getAtomSpace();
    Handle hkey = asp->add_atom(createNode(PREDICATE_NODE, key));
    ValuePtr vp = hcsn->getValue(hkey);
    if (nullptr == vp) return 0;

    if (vp->is_type(FLOAT_VALUE))
        return FloatValueCast(vp)->value()[0];

    if (vp->is_type(NUMBER_NODE))
        return NumberNodeCast(vp)->get_value();

    return 0;
}

CogServer::~CogServer()
{
    logger().debug("[CogServer] enter destructor");
    disableWebServer();
    disableNetworkServer();
    logger().debug("[CogServer] exit destructor");
}

CogServer::CogServer(void) :
    _consoleServer(nullptr),
    _webServer(nullptr),
    _mcpServer(nullptr),
    _running(false)
{
}

/// Allow at most `max_open_socks` concurrent connections.
/// Setting this larger than 10 or 20 will usually lead to
/// poor performance, and setting it larger than 140 will
/// require changing the unix ulimit on max open file descriptors.
/// (This is because, for each one that is counted here, there are
/// another half-dozen utility sockets and so 140*7 gets large and
/// overflows the ulimit max of 1024.)
void CogServer::set_max_open_sockets(int max_open_socks)
{
    _socket_manager.set_max_open_sockets(max_open_socks);
}

/// Open the given port number for network service.
void CogServer::enableNetworkServer(const Handle& hcsn)
{
    if (_consoleServer) return;
    int port = get_port(hcsn, "*-telnet-port-*");
    if (0 >= port) return;

    try
    {
        _consoleServer = new NetworkServer(port, "Telnet Server", &_socket_manager);
    }
    catch (const std::system_error& ex)
    {
        fprintf(stderr, "Error: Cannot enable network server at port %d: %s\n",
            port, ex.what());
        logger().info("Error: Cannot enable network server at port %d: %s\n",
            port, ex.what());
        std::rethrow_exception(std::current_exception());
    }

    auto make_console = [hcsn, this](SocketManager* mgr)->ServerSocket*
            { return new ServerConsole(hcsn, *this, mgr); };
    _consoleServer->run(make_console);
    logger().info("Network server running on port %d", port);
}

/// Open the given port number for web service.
void CogServer::enableWebServer(const Handle& hcsn)
{
#ifdef HAVE_OPENSSL
    if (_webServer) return;
    int port = get_port(hcsn, "*-web-port-*");
    if (0 >= port) return;

    try
    {
        _webServer = new NetworkServer(port, "WebSocket Server", &_socket_manager);
    }
    catch (const std::system_error& ex)
    {
        fprintf(stderr, "Error: Cannot enable web server at port %d: %s\n",
            port, ex.what());
        logger().info("Error: Cannot enable web server at port %d: %s\n",
            port, ex.what());
        std::rethrow_exception(std::current_exception());
    }

    auto make_console = [hcsn, this](SocketManager* mgr)->ServerSocket* {
        ServerSocket* ss = new WebServer(hcsn, *this, mgr);
        ss->act_as_http_socket();
        return ss;
    };
    _webServer->run(make_console);
    logger().info("Web server running on port %d", port);
#else
    printf("CogServer compiled without WebSockets.\n");
    logger().info("CogServer compiled without WebSockets.");
#endif // HAVE_SSL
}

/// Open the given port number for MCP service.
void CogServer::enableMCPServer(const Handle& hcsn)
{
#if HAVE_MCP
    if (_mcpServer) return;
    int port = get_port(hcsn, "*-mcp-port-*");
    if (0 >= port) return;

    try
    {
        _mcpServer = new NetworkServer(port, "Model Context Protocol Server", &_socket_manager);
    }
    catch (const std::system_error& ex)
    {
        fprintf(stderr, "Error: Cannot enable MCP server at port %d: %s\n",
            port, ex.what());
        logger().info("Error: Cannot enable MCP server at port %d: %s\n",
            port, ex.what());
        std::rethrow_exception(std::current_exception());
    }

    auto make_console = [hcsn, this](SocketManager* mgr)->ServerSocket* {
        ServerSocket* ss = new MCPServer(hcsn, mgr);
        ss->act_as_mcp();
        return ss;
    };
    _mcpServer->run(make_console);
    logger().info("MCP server running on port %d", port);
#else
    printf("CogServer compiled without MCP Support.\n");
    logger().info("CogServer compiled without MCP Support.");
#endif // HAVE_MCP
}

void CogServer::disableNetworkServer()
{
    // No-op for backwards-compat. Actual cleanup performed on
    // main-loop exit.  See notes there about thread races.
}

void CogServer::disableWebServer()
{
}

void CogServer::disableMCPServer()
{
}

void CogServer::stop()
{
    _running = false;

    // Cancel the request queue to wake up barrier() in serverLoop().
    requestQueue.cancel();
}

void CogServer::serverLoop()
{
    prctl(PR_SET_NAME, "cogserv:loop", 0, 0, 0);
    logger().info("Starting CogServer loop.");
    while (_running)
    {
        try
        {
            requestQueue.barrier();
            while (0 < getRequestQueueSize())
                runLoopStep();
        }
        catch (const concurrent_queue<Request*>::Canceled& ex)
        {
            break;
        }
    }

    // Reset queue cancellation immediately. Handler threads may
    // still be running and pushing requests; they must not see a
    // canceled queue. But also, we will need it in a non-cancelled
    // state in order to drain what's in there.
    requestQueue.cancel_reset();

    // Prevent the Network server from accepting any more connections,
    // and from queueing any more Requests.
    if (_mcpServer)
        _mcpServer->stop_listening();
    if (_webServer)
        _webServer->stop_listening();
    if (_consoleServer)
        _consoleServer->stop_listening();

    // Drain whatever is left in the queue.
    while (0 < getRequestQueueSize())
        processRequests();

    // Join handler threads. Do this after the drain to avoid deadlock.
    if (_mcpServer)
    {
        _mcpServer->join_threads();
        delete _mcpServer;
    }
    if (_webServer)
    {
        _webServer->join_threads();
        delete _webServer;
    }
    if (_consoleServer)
    {
        _consoleServer->join_threads();
        delete _consoleServer;
    }

    _mcpServer = nullptr;
    _webServer = nullptr;
    _consoleServer = nullptr;

    logger().info("Stopped CogServer");
    logger().flush();
}

void CogServer::runLoopStep(void)
{
    // Process requests
    if (0 < getRequestQueueSize())
        processRequests();
}

std::string CogServer::display_stats(int nlines)
{
    if (_consoleServer)
        return _socket_manager.display_stats_full(
            _consoleServer->get_name(), _consoleServer->get_start_time(), nlines);
    else
        return "Console server is not running";
}

std::string CogServer::display_web_stats(void)
{
    if (_webServer)
        return _socket_manager.display_stats_full(
            _webServer->get_name(), _webServer->get_start_time());
    else
        return "Web server is not running";
}

std::string CogServer::stats_legend(void)
{
	return
       "The current date in UTC is printed, followed by:\n"
       "  up-since: the date when the server was started.\n"
       "  last: the date when the most recent connection was opened.\n"
       "  tot-cnct: grand total number of network connections opened.\n"
       "  cur-open-socks: number of currently open connections.\n"
       "  num-open-fds: number of open file descriptors.\n"
       "  stalls: times that open stalled due to hitting max-open-cnt.\n"
       "  tot-lines: total number of newlines received by all shells.\n"
       "  cpu user sys: number of CPU seconds used by server.\n"
       "  maxrss: resident set size, in KB. Taken from `getrusage`.\n"
       "\n"
       "The table shows a list of the currently open connections.\n"
       "The table header has the following form:\n"
       "OPEN-DATE THREAD STATE NLINE LAST-ACTIVITY K U SHEL QZ E PENDG\n"
       "The columns are:\n"
       "  OPEN-DATE -- when the connection was opened.\n"
       "  THREAD -- the Linux thread-id, as printed by `ps -eLf`\n"
       "  STATE -- several states possible; `iwait` means waiting for input.\n"
       "  NLINE -- number of newlines received by the shell.\n"
       "  LAST-ACTIVITY -- the last time anything was received.\n"
       "  K -- socket kind. `T` for telnet, `W` for WebSocket,\n"
       "                    `H` for http, 'M' for MCP.\n"
       "  U -- use count. The number of active handlers for the socket.\n"
       "  SHEL -- the current shell processor for the socket.\n"
       "  QZ -- size of the unprocessed (pending) request queue.\n"
       "  E -- `T` if the shell evaluator is running, else `F`.\n"
       "  PENDG -- number of bytes of output not yet sent.\n"
       "\n";
}

// =============================================================
