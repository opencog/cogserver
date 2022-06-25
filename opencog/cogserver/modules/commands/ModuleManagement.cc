/*
 * opencog/cogserver/modules/commands/ModuleManagement.cc
 *
 * Copyright (C) 2008 by OpenCog Foundation
 * Copyright (C) 2022 Linas Vepstas
 * Written by Gustavo Gama <gama@vettalabs.com>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#include <sstream>
#include <string>

#include <opencog/cogserver/modules/commands/ModuleManagement.h>

using namespace opencog;

// ====================================================================

const RequestClassInfo&
ListModulesRequest::info(void)
{
    static const RequestClassInfo _cci(
        "listmodules",
        "List the currently loaded modules",
        "Usage: listmodules\n\n"
        "List modules currently loaded into the cogserver. "
    );
    return _cci;
}

bool ListModulesRequest::execute()
{
    std::string moduleList = _cogserver.listModules();
    send(moduleList);
    return true;
}

// ====================================================================

const RequestClassInfo&
LoadModuleRequest::info()
{
    static const RequestClassInfo _cci(
        "loadmodule",
        "Load an opencog module",
        "Usage: loadmodule <module>\n\n"
        "Load the named opencog module"
    );
    return _cci;
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

// ====================================================================

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

// ====================================================================
