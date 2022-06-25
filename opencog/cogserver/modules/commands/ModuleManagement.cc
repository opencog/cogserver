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
ConfigModuleRequest::info(void)
{
    static const RequestClassInfo _cci(
        "config",
        "Config a loaded module",
        "Usage: config <module> <config-string>\n\n"
        "Config the indicated module. The module can be specified\n"
        "either as the shared-lib filename, or as the module id.\n"
        "Both of these are shown by the `list` command.\n\n"
        "The configuration string is passed to those modules that\n"
        "support configuration, and is interpreted in a way that is\n"
        "specific to that module. Most modules do not need (or support)\n"
        "configuration.\n\n"
        "The `sexpr` module uses configuration strings to set up proxy\n"
        "modes. In the write-through proxy mode, data sent to the\n"
        "cogserver can be forwarded to other servers, or written to\n"
        "local disk storage.\n"
    );
    return _cci;
}

bool ConfigModuleRequest::execute()
{
    logger().debug("[ConfigModuleRequest] execute");
    std::ostringstream oss;
    if (2 != _parameters.size()) {
        oss << "invalid syntax: config <module> <config-string>" << std::endl;
        send(oss.str());
        return false;
    }
    std::string filename = _parameters.front();
    _parameters.pop_front();
    std::string cfg = _parameters.front();
    if (_cogserver.configModule(filename, cfg)) {
        oss << "done" << std::endl;
        send(oss.str());
        return true;
    }
    oss << "Unable to config module \"" << filename
        << "\". Check the server logs for details." << std::endl;
    send(oss.str());
    return false;
}

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

    if (_cogserver.loadModule(filename)) {
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
