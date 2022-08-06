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

#include "CogServer.h"
#include "BaseServer.h"

using namespace opencog;

CogServer::~CogServer()
{
    logger().debug("[CogServer] enter destructor");
    disableWebServer();
    disableNetworkServer();
    logger().debug("[CogServer] exit destructor");
}

CogServer::CogServer(void) :
    BaseServer(),
    _consoleServer(nullptr),
    _webServer(nullptr),
    _running(false)
{
	set_max_open_sockets();
}

CogServer::CogServer(AtomSpacePtr as) :
    BaseServer(as),
    _consoleServer(nullptr),
    _webServer(nullptr),
    _running(false)
{
	set_max_open_sockets();
}

/// Allow at most `max_open_socks` concurrent connections.
/// Setting this larger than 10 or 20 will usually lead to
/// poor performance, and setting it larger than 140 will
/// require changing the unix ulimit on max open file descriptors.
void CogServer::set_max_open_sockets(int max_open_socks)
{
    ServerSocket::set_max_open_sockets(max_open_socks);
}

/// Open the given port number for network service.
void CogServer::enableNetworkServer(int port)
{
    if (_consoleServer) return;
    _consoleServer = new NetworkServer(port);

    auto make_console = [](void)->ServerSocket*
            { return new ServerConsole(); };
    _consoleServer->run(make_console);
    _running = true;
    logger().info("Network server running on port %d", port);
}

/// Open the given port number for web service.
void CogServer::enableWebServer(int port)
{
    if (_webServer) return;
    _webServer = new NetworkServer(port);

    auto make_console = [](void)->ServerSocket*
            { return new WebServer(); };
    _webServer->run(make_console);
    _running = true;
    logger().info("Web server running on port %d", port);
}

void CogServer::disableNetworkServer()
{
    // No-op for backwards-compat. Actual cleanup performed on
    // main-loop exit.  See notes there about thread races.
}

void CogServer::disableWebServer()
{
}

void CogServer::stop()
{
    _running = false;
}

void CogServer::serverLoop()
{
    prctl(PR_SET_NAME, "cogserv:loop", 0, 0, 0);
    logger().info("Starting CogServer loop.");
    while (_running)
    {
        while (0 < getRequestQueueSize())
            runLoopStep();

        // XXX FIXME. terrible terrible hack. What we should be
        // doing is running in our own thread, waiting on a semaphore,
        // until some request is queued. Spinning is .. just wrong.
        usleep(20000);
    }

    // Prevent the Network server from accepting any more connections,
    // and from queing any more Requests. I think. This might be racey.
    _webServer->stop();
    _consoleServer->stop();

    // Drain whatever is left in the queue.
    while (0 < getRequestQueueSize())
        processRequests();

    // We need to clean up in the same thread where we are looping;
    // doing this in other threads, e.g. the thread that calls stop()
    // or the thread that calls disableNetworkServer() will lead to
    // races.
    delete _webServer;
    _webServer = nullptr;
    delete _consoleServer;
    _consoleServer = nullptr;

    logger().info("Stopped CogServer");
    logger().flush();
}

void CogServer::runLoopStep(void)
{
    // Process requests
    if (0 < getRequestQueueSize())
    {
        struct timeval timer_start, timer_end, requests_time;
        gettimeofday(&timer_start, NULL);
        processRequests();
        gettimeofday(&timer_end, NULL);
        timersub(&timer_end, &timer_start, &requests_time);

        logger().fine("[CogServer::runLoopStep] Time to process requests: %f",
                   requests_time.tv_usec/1000000.0
                  );
    }
}

std::string CogServer::display_stats(void)
{
    return _consoleServer->display_stats();
}

// =============================================================
// Singleton instance stuff.
//
// I don't really like singleton instances very much. There are some
// interesting use cases where one might want to run multipel
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
