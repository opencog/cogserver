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

#include <opencog/cogserver/modules/ModuleManagement.h>

using namespace opencog;

// ====================================================================

const RequestClassInfo&
ListModulesRequest::info(void)
{
    static const RequestClassInfo _cci(
        "list",
        "List the currently loaded cogserver modules",
        "Usage: list\n\n"
        "List modules currently loaded into the cogserver.\n"
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
        "load",
        "Load a cogserver module",
        "Usage: load <module>\n\n"
        "Load the named cogserver module. The module name must be\n"
        "specified as the shared-library filename. The full directory\n"
        "path, starting with a leading slash, must be provided!\n"
    );
    return _cci;
}

bool LoadModuleRequest::execute()
{
    logger().debug("[LoadModuleRequest] execute");
    std::ostringstream oss;
    if (_parameters.empty()) {
        oss << "invalid syntax: load <filename>" << std::endl;
        send(oss.str());
        return false;
    }
    std::string& filename = _parameters.front();

    if (_cogserver.loadModule(filename, _cogserver.getHandle())) {
        oss << "done" << std::endl;
        send(oss.str());
        return true;
    }
    oss << "Unable to load module \"" << filename
        << "\". Check the server logs for details." << std::endl;
    send(oss.str());
    return false;
}

// ====================================================================

const RequestClassInfo&
UnloadModuleRequest::info(void)
{
    static const RequestClassInfo _cci(
        "unload",
        "Unload an opencog module",
        "Usage: unload <module>\n\n"
        "Unload the indicated module. The module can be specified\n"
        "either as the shared-lib filename, or as the module id.\n"
        "Both of these are shown by the `list` command.\n"
    );
    return _cci;
}

bool UnloadModuleRequest::execute()
{
    logger().debug("[UnloadModuleRequest] execute");
    std::ostringstream oss;
    if (_parameters.empty()) {
        oss << "invalid syntax: unload <filename> | <module id>" << std::endl;
        send(oss.str());
        return false;
    }
    std::string& filename = _parameters.front();
    if (_cogserver.unloadModule(filename)) {
        oss << "done" << std::endl;
        send(oss.str());
        return true;
    }
    oss << "Unable to unload module \"" << filename
        << "\". Check the server logs for details." << std::endl;
    send(oss.str());
    return false;
}

// ====================================================================
