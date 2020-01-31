/*
 * opencog/cogserver/modules/commands/ShutdownRequest.h
 *
 * Copyright (C) 2008 by OpenCog Foundation
 * Written by Gustavo Gama <gama@vettalabs.com>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef _OPENCOG_SHUTDOWN_REQUEST_H
#define _OPENCOG_SHUTDOWN_REQUEST_H

#include <vector>
#include <string>

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

    static inline const RequestClassInfo& info() {
        static const RequestClassInfo _cci(
            "shutdown",
            "Shut down the cogserver",
            "Usage: shutdown\n\n"
            "Halt the cogserver in an  orderly fashion"
        );
        return _cci;
    }

    ShutdownRequest(CogServer&);
    virtual ~ShutdownRequest();
    virtual bool execute(void);
    virtual bool isShell(void) {return info().is_shell;}
};

/** @}*/
} // namespace 

#endif // _OPENCOG_SHUTDOWN_REQUEST_H
