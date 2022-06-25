/*
 * opencog/cogserver/modules/commands/UnloadModuleRequest.h
 *
 * Copyright (C) 2008 by OpenCog Foundation
 * Written by Gustavo Gama <gama@vettalabs.com>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef _OPENCOG_UNLOAD_MODULE_REQUEST_H
#define _OPENCOG_UNLOAD_MODULE_REQUEST_H

#include <string>
#include <vector>

#include <opencog/cogserver/server/Request.h>
#include <opencog/cogserver/server/RequestClassInfo.h>

namespace opencog
{
/** \addtogroup grp_server
 *  @{
 */

class UnloadModuleRequest : public Request
{
public:
    UnloadModuleRequest(CogServer&);
    virtual ~UnloadModuleRequest();
    static const RequestClassInfo& info(void);
    virtual bool execute(void);
    virtual bool isShell(void) {return info().is_shell;}
};

/** @}*/
} // namespace 

#endif // _OPENCOG_UNLOAD_MODULE_REQUEST_H
