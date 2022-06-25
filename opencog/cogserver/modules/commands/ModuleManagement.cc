

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

using namespace opencog;

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

