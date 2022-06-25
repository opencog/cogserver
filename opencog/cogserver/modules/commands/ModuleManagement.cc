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

#include <opencog/cogserver/server/CogServer.h>
#include <opencog/cogserver/server/Request.h>
#include <opencog/cogserver/server/RequestClassInfo.h>

#define DEFINE_REQUEST(REQUESTNAME)                                   \
                                                                      \
namespace opencog                                                     \
{                                                                     \
class REQUESTNAME : public Request {                                  \
public:                                                               \
    REQUESTNAME(CogServer&);                                          \
    virtual ~REQUESTNAME();                                           \
    static const RequestClassInfo& info(void);                        \
    virtual bool execute(void);                                       \
    virtual bool isShell(void) {return info().is_shell;}              \
}; }


DEFINE_REQUEST(ListModulesRequest)
DEFINE_REQUEST(LoadModuleRequest)

using namespace opencog;

// ====================================================================
ListModulesRequest::ListModulesRequest(CogServer& cs) : Request(cs) {}
ListModulesRequest::~ListModulesRequest() {}

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
LoadModuleRequest::LoadModuleRequest(CogServer& cs) : Request(cs) {}
LoadModuleRequest::~LoadModuleRequest() {}

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
