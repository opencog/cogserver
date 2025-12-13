/*
 * opencog/cogserver/modules/commands/ShutdownRequest.cc
 *
 * Copyright (C) 2008 by OpenCog Foundation
 * Written by Gustavo Gama <gama@vettalabs.com>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <opencog/util/oc_assert.h>
#include <opencog/cogserver/server/CogServer.h>
#include <opencog/network/ConsoleSocket.h>

#include "ShutdownRequest.h"

using namespace opencog;

ShutdownRequest::ShutdownRequest(CogServer& cs) : Request(cs) {}
ShutdownRequest::~ShutdownRequest() {}

const RequestClassInfo&
ShutdownRequest::info(void)
{
    static const RequestClassInfo _cci(
        "shutdown",
        "Shut down the cogserver",
        "Usage: shutdown\n\n"
        "Halt the cogserver in an  orderly fashion"
    );
    return _cci;
}

bool ShutdownRequest::execute()
{
    std::ostringstream oss;
    oss << "Shutting down cogserver" << std::endl;
    send(oss.str());

    _cogserver.stop();

    ConsoleSocket* con = get_console();
    OC_ASSERT(con, "Bad request state");
    set_console(nullptr);
    con->Exit();

    return true;
}
