#include "ListModulesRequest.h"
#include <opencog/cogserver/server/CogServer.h>

using namespace opencog;

ListModulesRequest::ListModulesRequest(CogServer& cs) : Request(cs)
{
}

ListModulesRequest::~ListModulesRequest()
{
    logger().debug("[ListModulesRequest] destructor");
}

bool ListModulesRequest::execute()
{
    std::string moduleList = _cogserver.listModules();
    send(moduleList);
    return true;
}

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
