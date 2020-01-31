/*
 * opencog/cogserver/modules/commands/ListRequest.h
 *
 * Copyright (C) 2008 by OpenCog Foundation
 * Written by Gustavo Gama <gama@vettalabs.com>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef _OPENCOG_LIST_REQUEST_H
#define _OPENCOG_LIST_REQUEST_H

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

class ListRequest : public Request
{

protected:

    HandleSeq _handles;
    std::ostringstream  _error;

    void sendOutput(void);
    void sendError (void);
    bool syntaxError(void);

public:

    static inline const RequestClassInfo& info() {
        static const RequestClassInfo _cci(
            "list",
            "List atoms in the atomtable",
            "Usage: list [[-h <handle>] | [-n <name>] [[-t|-T] <type>]] [-m <num>] \n\n"
            "List atoms in the atomtable. Optional flags are:\n"
            "   -h <handle>: list the atom identified by the specified handle\n"
            "   -n <name>:   list the nodes identified by the specified name\n"
            "   -t <name>:   list the atoms of the specified type\n"
            "   -T <name>:   list the atoms of the specified type (including subtypes)\n"
            "   -m <num>:    list the nodes up to the specified size"
        );
        return _cci;
    }

    ListRequest(CogServer&);
    virtual ~ListRequest();
    virtual bool execute(void);
    virtual bool isShell(void) {return info().is_shell;}
};

/** @}*/
} // namespace 

#endif // _OPENCOG_LIST_REQUEST_H
