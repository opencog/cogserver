/*
 * opencog/network/ConsoleSocket.cc
 *
 * Copyright (C) 2002-2007 Novamente LLC
 * Written by Andre Senna <senna@vettalabs.com>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <string>

#include <opencog/util/Config.h>
#include <opencog/util/Logger.h>
#include <opencog/util/misc.h>

#include <opencog/network/ConsoleSocket.h>

using namespace opencog;

ConsoleSocket::ConsoleSocket(void)
{
    _shell = nullptr;
    _use_count = 0;
}

ConsoleSocket::~ConsoleSocket()
{
    logger().debug("[ConsoleSocket] destructor");

    // We need the use-count and the condition variables because
    // the design of boost:asio is broken. Basically, the boost::asio
    // code calls this destructor for ConsoleSocket while there are
    // still requests outstanding in another thread.  We have to stall
    // the destructor until all the in-flight requests are complete;
    // we use the condition variable to do this.
    //
    // Some details: basically, the remote end of the socket "fires and
    // forgets" a bunch of commands, and then closes the socket before
    // these requests have completed.  boost:asio notices that the
    // remote socket has closed, and so decides its a good day to call
    // destructors. But of course, its not ... because the requests
    // still have to be handled.
    std::unique_lock<std::mutex> lck(_in_use_mtx);
    while (_use_count) _in_use_cv.wait(lck);
    lck.unlock();

    // If there's a shell, kill it.
    if (_shell) delete _shell;

    logger().debug("[ConsoleSocket] destructor finished");
}

void ConsoleSocket::SetShell(GenericShell *g)
{
    _shell = g;

	// Push out a new prompt, when the shell closes.
	if (nullptr == g) OnLine("");
}

// ==================================================================

std::string ConsoleSocket::connection_header(void)
{
    return ServerSocket::connection_header() + " U SHEL QZ E PENDG";
}

std::string ConsoleSocket::connection_stats(void)
{
    std::string rc = ServerSocket::connection_stats();

    char buf[40];
    snprintf(buf, 40, " %1d ", get_use_count());
    rc += buf;

    if (_shell)
    {
        rc += _shell->_name;
        snprintf(buf, 40, " %2zd %c %5zd",
            _shell->queued(), _shell->eval_done()?'F':'T',
            _shell->pending());
        rc += buf;
    }
    else rc += "cogs           ";

    return rc;
}

// ==================================================================
