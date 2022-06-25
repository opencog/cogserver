/*
 * opencog/cogserver/modules/commands/UnloadModuleRequest.cc
 *
 * Copyright (C) 2008 by OpenCog Foundation
 * Written by Gustavo Gama <gama@vettalabs.com>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <vector>
#include <sstream>

#include "UnloadModuleRequest.h"
#include <opencog/cogserver/server/CogServer.h>

using namespace opencog;

UnloadModuleRequest::UnloadModuleRequest(CogServer& cs) : Request(cs) {}
UnloadModuleRequest::~UnloadModuleRequest() {}

const RequestClassInfo&
UnloadModuleRequest::info(void)
{
    static const RequestClassInfo _cci(
        "unloadmodule",
        "Unload an opencog module",
        "Usage: unload <module>\n\n"
        "Unload the indicated module."
    );
    return _cci;
}

bool UnloadModuleRequest::execute()
{
    logger().debug("[UnloadModuleRequest] execute");
    std::ostringstream oss;
    if (_parameters.empty()) {
        oss << "invalid syntax: unloadmodule [<filename> | <module id>]" << std::endl;
        send(oss.str());
        return false;
    }
    std::string& filename = _parameters.front();
    if (_cogserver.unloadModule(filename)) {
        oss << "done" << std::endl;
        send(oss.str());
        return true;
    } else {
        oss << "Unable to unload module \"" << filename << "\". Check the server logs for details." << std::endl;
        send(oss.str());
        return false;
    }
}
