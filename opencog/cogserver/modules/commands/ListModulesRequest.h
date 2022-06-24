#ifndef _OPENCOG_LIST_MODULES_REQUEST_H
#define _OPENCOG_LIST_MODULES_REQUEST_H

#include <sstream>
#include <string>
#include <vector>

#include <opencog/atoms/base/Handle.h>
#include <opencog/cogserver/server/Request.h>
#include <opencog/cogserver/server/RequestClassInfo.h>

namespace opencog
{
/** \addtogroup grp_server
 *  @{
 */

class ListModulesRequest : public Request
{
public:
    ListModulesRequest(CogServer&);
    virtual ~ListModulesRequest();
    static const RequestClassInfo& info(void);
    virtual bool execute(void);
    virtual bool isShell(void) {return info().is_shell;}
};

/** @}*/
} // namespace

#endif // _OPENCOG_LIST_MODULES_REQUEST_H
