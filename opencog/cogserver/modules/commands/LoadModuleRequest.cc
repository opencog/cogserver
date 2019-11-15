/*
 * opencog/cogserver/modules/commands/LoadModuleRequest.cc
 *
 * Copyright (C) 2008 by OpenCog Foundation
 * Written by Gustavo Gama <gama@vettalabs.com>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "LoadModuleRequest.h"

#include <vector>
#include <sstream>

#include <opencog/cogserver/server/CogServer.h>
#include <opencog/util/Logger.h>
#include <opencog/util/exceptions.h>

using namespace opencog;

LoadModuleRequest::LoadModuleRequest(CogServer& cs) : Request(cs)
{
}

LoadModuleRequest::~LoadModuleRequest()
{
}

bool LoadModuleRequest::execute()
{
    logger().debug("[LoadModuleRequest] execute");
    std::ostringstream oss;
    if (_parameters.empty()) {
        oss << "invalid syntax: loadmodule <filename>" << std::endl;
        send(oss.str());
        return false;
    }
    std::string& filename = _parameters.front();

    if (_cogserver.loadModule(filename)) {
        oss << "done" << std::endl;
        send(oss.str());
        return true;
    } else {
        oss << "Unable to load module \"" << filename << "\". Check the server logs for details." << std::endl;
        send(oss.str());
        return false;
    }
}
