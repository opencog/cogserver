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

#include <opencog/util/Config.h>
#include <opencog/util/Logger.h>
#include <opencog/util/misc.h>
#include <opencog/util/platform.h>

#include <opencog/atomspace/AtomSpace.h>
#include <opencog/network/NetworkServer.h>

#include <opencog/cogserver/server/ServerConsole.h>

#include "CogServer.h"
#include "BaseServer.h"

using namespace opencog;

BaseServer* CogServer::createInstance(AtomSpace* as)
{
    return new CogServer(as);
}

CogServer::~CogServer()
{
    logger().debug("[CogServer] enter destructor");
    disableNetworkServer();
    logger().debug("[CogServer] exit destructor");
}

CogServer::CogServer(AtomSpace* as) :
    BaseServer(as),
    _networkServer(nullptr),
    _running(false)
{
}

void CogServer::enableNetworkServer(int port)
{
    if (_networkServer) return;
    _networkServer = new NetworkServer(config().get_int("SERVER_PORT", port));

    auto make_console = [](void)->ConsoleSocket*
	     { return new ServerConsole(); };
    _networkServer->run(make_console);
    _running = true;
}

void CogServer::disableNetworkServer()
{
    stop();
    if (_networkServer)
    {
        delete _networkServer;
        _networkServer = nullptr;
    }
}

void CogServer::stop()
{
    // Prevent the Network server from accepting any more connections,
    // and from queing any more Requests. I think. This might be racey.
    if (_networkServer) _networkServer->stop();

    _running = false;

    // Drain whatever is left in the queue.
    while (0 < getRequestQueueSize())
        processRequests();
}

void CogServer::serverLoop()
{
    logger().info("Starting CogServer loop.");
    while(_running)
    {
        while (0 < getRequestQueueSize())
            runLoopStep();

        // XXX FIXME. terrible terrible hack. What we should be
        // doing is running in our own thread, waiting on a semaphore,
        // until some request is queued. Spinning is .. just wrong.
        usleep(20000);
    }

    // No way to process requests. Stop accepting network connections.
    disableNetworkServer();
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

// =============================================================
