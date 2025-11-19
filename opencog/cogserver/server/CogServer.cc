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

#include <opencog/atomspace/AtomSpace.h>
#include <opencog/network/NetworkServer.h>

#include <opencog/cogserver/server/ServerConsole.h>
#include <opencog/cogserver/server/WebServer.h>
#include <opencog/cogserver/server/MCPServer.h>

#include "CogServer.h"

using namespace opencog;

CogServer::~CogServer()
{
    logger().debug("[CogServer] enter destructor");
    disableWebServer();
    disableNetworkServer();
    logger().debug("[CogServer] exit destructor");
}

CogServer::CogServer(void) :
    _atomSpace(createAtomSpace()),
    _consoleServer(nullptr),
    _webServer(nullptr),
    _mcpServer(nullptr),
    _running(false)
{
}

CogServer::CogServer(AtomSpacePtr as) :
    _atomSpace(as),
    _consoleServer(nullptr),
    _webServer(nullptr),
    _mcpServer(nullptr),
    _running(false)
{
    if (nullptr == as)
        _atomSpace = createAtomSpace();
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
void CogServer::enableNetworkServer(int port)
{
    if (_consoleServer) return;
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

    auto make_console = [](SocketManager* mgr)->ServerSocket*
            { return new ServerConsole(cogserver(), mgr); };
    _consoleServer->run(make_console);
    _running = true;
    logger().info("Network server running on port %d", port);
}

/// Open the given port number for web service.
void CogServer::enableWebServer(int port)
{
#ifdef HAVE_OPENSSL
    if (_webServer) return;
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

    auto make_console = [](SocketManager* mgr)->ServerSocket* {
        ServerSocket* ss = new WebServer(cogserver(), mgr);
        ss->act_as_http_socket();
        return ss;
    };
    _webServer->run(make_console);
    _running = true;
    logger().info("Web server running on port %d", port);
#else
    printf("CogServer compiled without WebSockets.\n");
    logger().info("CogServer compiled without WebSockets.");
#endif // HAVE_SSL
}

/// Open the given port number for MCP service.
void CogServer::enableMCPServer(int port)
{
#if HAVE_MCP
    if (_mcpServer) return;
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

    auto make_console = [](SocketManager* mgr)->ServerSocket* {
        ServerSocket* ss = new MCPServer(cogserver(), mgr);
        ss->act_as_mcp();
        return ss;
    };
    _mcpServer->run(make_console);
    _running = true;
    logger().info("MCP server running on port %d", port);
#else
    printf("CogServer compiled without MCP Support.\n");
    logger().info("CogServer compiled without MCP Support.");
#endif // HAVE_SSL
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

    // Prevent the Network server from accepting any more connections,
    // and from queueing any more Requests. I think. This might be racey.
    if (_mcpServer)
        _mcpServer->stop();
    if (_webServer)
        _webServer->stop();
    if (_consoleServer)
        _consoleServer->stop();

    // Reset queue cancellation to allow draining remaining requests.
    requestQueue.cancel_reset();

    // Drain whatever is left in the queue.
    while (0 < getRequestQueueSize())
        processRequests();

    // We need to clean up in the same thread where we are looping;
    // doing this in other threads, e.g. the thread that calls stop()
    // or the thread that calls disableNetworkServer() will lead to
    // races.
    if (_mcpServer) delete _mcpServer;
    _mcpServer = nullptr;
    if (_webServer) delete _webServer;
    _webServer = nullptr;
    if (_consoleServer) delete _consoleServer;
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
// Singleton instance stuff.
//
// I don't really like singleton instances very much. There are some
// interesting use cases where one might want to run multiple
// cogservers. However, at this time, too much of the code (???)
// assumes a singleton instance, so we leave this for now. XXX FIXME.

// The guile module needs to be able to delete this singleton.
// So put it where the guile module can find it.
namespace opencog
{
    CogServer* serverInstance = nullptr;
};

CogServer& opencog::cogserver(void)
{
    if (nullptr == serverInstance)
        serverInstance = new CogServer();

    return *serverInstance;
}

CogServer& opencog::cogserver(AtomSpacePtr as)
{
    if (nullptr == serverInstance)
        serverInstance = new CogServer(as);

    return *serverInstance;
}

// =============================================================
