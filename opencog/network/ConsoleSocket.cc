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

// _max_open_sockets is the largest number of concurrently open
// sockets we will allow in the server. Currently set to 60.
// Note that each SchemeShell (actually, SchemeEval) will open
// another half-dozen pipes and what-not, so actually, the number
// of open files will increase by 4 or 6 or so for each network
// connection. With the default `ulimit -a` of 1024 open files,
// this should work OK (including open files for the logger, the
// databases, etc.).
//
// July 2019 - change to 10. When it is 60, it just thrashes like
// crazy, mostly because there are 60 threads thrashing in guile
// on some lock. And that's pretty pointless...
unsigned int ConsoleSocket::_max_open_sockets = 10;
volatile unsigned int ConsoleSocket::_num_open_sockets = 0;
std::mutex ConsoleSocket::_max_mtx;
std::condition_variable ConsoleSocket::_max_cv;

ConsoleSocket::ConsoleSocket(void)
{
    _shell = nullptr;
    _use_count = 0;

    // Block here, if there are too many concurrently-open sockets.
    std::unique_lock<std::mutex> lck(_max_mtx);
    _num_open_sockets++;

    // If we are just below the max limit, send a half-ping in an
    // attempt to force any half-open connections to close.
    if (_max_open_sockets <= _num_open_sockets)
        half_ping();
    while (_max_open_sockets < _num_open_sockets) _max_cv.wait(lck);
}

ConsoleSocket::~ConsoleSocket()
{
    logger().debug("[ConsoleSocket] destructor");

    // We need the use-count and the condition variables because
    // somehow the design of either this subsystem, or boost:asio
    // is broken. Basically, the boost::asio code calls this destructor
    // for ConsoleSocket while there are still requests outstanding
    // in another thread.  We have to stall the destructor until all
    // the in-flight requests are complete; we use the condition
    // variable to do this. But really, something somewhere is broken
    // or mis-designed. Not sure what/where; this code is too complicated.
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

    std::unique_lock<std::mutex> mxlck(_max_mtx);
    _num_open_sockets--;
    _max_cv.notify_all();
    mxlck.unlock();

    logger().debug("[ConsoleSocket] destructor finished");
}

void ConsoleSocket::SetShell(GenericShell *g)
{
    _shell = g;
}

// ==================================================================

std::string ConsoleSocket::connection_header(void)
{
    return ServerSocket::connection_header() + " U SHEL QZ E PENDG";
}

std::string ConsoleSocket::connection_stats(void)
{
    char buf[40];
    snprintf(buf, 40, " %1d ", get_use_count());

    std::string rc = ServerSocket::connection_stats() + buf;

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
