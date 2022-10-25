/*
 * opencog/cogserver/server/BaseServer.h
 *
 * Copyright (C) 2008 by OpenCog Foundation
 * Written by Gustavo Gama <gama@vettalabs.com>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef _OPENCOG_BASE_SERVER_H
#define _OPENCOG_BASE_SERVER_H

#include <opencog/atomspace/AtomSpace.h>

namespace opencog
{
/** \addtogroup grp_server
 *  @{
 */
class BaseServer
{
protected:
    AtomSpacePtr _atomSpace;

public:
    /** Returns the atomspace instance. */
    AtomSpacePtr getAtomSpace() { return _atomSpace; }
    void setAtomSpace(const AtomSpacePtr& as) { _atomSpace = as; }

    BaseServer(void);
    BaseServer(AtomSpacePtr);
    virtual ~BaseServer();
}; // class


/** @}*/
}  // namespace

#endif // _OPENCOG_BASE_SERVER_H
