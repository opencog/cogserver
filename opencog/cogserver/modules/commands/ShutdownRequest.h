/*
 * opencog/cogserver/modules/commands/ShutdownRequest.h
 *
 * Copyright (C) 2008 by OpenCog Foundation
 * Written by Gustavo Gama <gama@vettalabs.com>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef _OPENCOG_SHUTDOWN_REQUEST_H
#define _OPENCOG_SHUTDOWN_REQUEST_H

#include <opencog/cogserver/server/Request.h>
#include <opencog/cogserver/server/RequestClassInfo.h>

namespace opencog
{
/** \addtogroup grp_server
 *  @{
 */

class ShutdownRequest : public Request
{
public:
    ShutdownRequest(CogServer&);
    virtual ~ShutdownRequest();
    static const RequestClassInfo& info();
    virtual bool execute(void);
    virtual bool isShell(void) {return info().is_shell;}
};

/** @}*/
} // namespace 

#endif // _OPENCOG_SHUTDOWN_REQUEST_H
