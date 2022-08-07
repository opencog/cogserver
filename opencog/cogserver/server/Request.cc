/*
 * opencog/cogserver/server/Request.cc
 *
 * Copyright (C) 2008 by OpenCog Foundation
 * Written by Gustavo Gama <gama@vettalabs.com>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * The Cogserver Request system is deprecated; users are encouraged to
 * explore writing guile (scheme) or python modules instead.
 */

#include <opencog/util/exceptions.h>
#include <opencog/util/Logger.h>
#include <opencog/util/oc_assert.h>

#include <opencog/network/ConsoleSocket.h>
#include <opencog/cogserver/server/ServerConsole.h>

#include "Request.h"

using namespace opencog;

Request::Request(CogServer& cs) :
    _console(nullptr), _cogserver(cs)
{
}

Request::~Request()
{
    logger().debug("[Request] destructor");
    if (_console)
    {
        // Send out a prompt to telnet users.
        //... Except for shells, which provide their own prompt.
        if (nullptr == _console->getShell())
        {
            ServerConsole* sc = dynamic_cast<ServerConsole*>(_console);
            if (sc) sc->sendPrompt();
        }
        _console->put();  // dec use count we are done with it.
    }
}

void Request::set_console(ConsoleSocket* con)
{
    // The "exit" request causes the console to be destroyed,
    // rendering the _console pointer invalid. However, generic
    // code will try to call Request::send() afterwards. So
    // prevent the invalid reference by zeroing he pointer.
    if (nullptr == con)
    {
        _console->put();  // dec use count -- we are done with socket.
        _console = nullptr;
        return;
    }

    OC_ASSERT(nullptr == _console, "Setting console twice!");

    logger().debug("[Request] setting socket: %p", con);
    con->get();  // inc use count -- we plan to use the socket
    _console = con;
}

void Request::send(const std::string& msg) const
{
    // The _console might be zero for the exit request, because the
    // exit command destroys the socket, and then tries to send a
    // reply on the socket it just destroyed.
    if (_console) _console->Send(msg);
}

void Request::setParameters(const std::list<std::string>& params)
{
    _parameters.assign(params.begin(), params.end());
}

void Request::addParameter(const std::string& param)
{
    _parameters.push_back(param);
}
