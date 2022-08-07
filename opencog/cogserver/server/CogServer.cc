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

#include "CogServer.h"
#include "BaseServer.h"

using namespace opencog;

CogServer::~CogServer()
{
    logger().debug("[CogServer] enter destructor");
    disableNetworkServer();
    logger().debug("[CogServer] exit destructor");
}

CogServer::CogServer(void) :
    BaseServer(),
    _networkServer(nullptr),
    _running(false)
{
}

CogServer::CogServer(AtomSpacePtr as) :
    BaseServer(as),
    _networkServer(nullptr),
    _running(false)
{
}

/// Open the given port number for network service.
/// Allow at most `max_open_socks` concurrent connections.
/// Setting this larger than 10 or 20 will usually lead to
/// poor performance, and setting it larger than 140 will
/// require changing the unix ulimit on max open file descriptors.
void CogServer::enableNetworkServer(int port, int max_open_socks)
{
    if (_networkServer) return;
    _networkServer = new NetworkServer(port);

    ServerSocket::set_max_open_sockets(max_open_socks);
    auto make_console = [](void)->ServerSocket*
            { return new ServerConsole(); };
    _networkServer->run(make_console);
    _running = true;
    logger().info("Network server running on port %d", port);
}

void CogServer::disableNetworkServer()
{
    // No-op for backwards-compat. Actual cleanup performed on
    // main-loop exit.  See notes there about thread races.
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
    _networkServer->stop();

    // Drain whatever is left in the queue.
    while (0 < getRequestQueueSize())
        processRequests();

    // We need to clean up in the same thread where we are looping;
    // doing this in other threads, e.g. the thread that calls stop()
    // or the thread that calls disableNetworkServer() will lead to
    // races.
    delete _networkServer;
    _networkServer = nullptr;

    logger().info("Stopped CogServer");
    logger().flush();
}

void CogServer::runLoopStep(void)
{
    // Process requests
    if (0 < getRequestQueueSize())
        processRequests();
}

std::string CogServer::display_stats(void)
{
    return _networkServer->display_stats();
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
