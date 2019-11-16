/*
 * opencog/cogserver/server/BaseServer.cc
 *
 * Copyright (C) 2002-2007 Novamente LLC
 * Written by Andre Senna <senna@vettalabs.com>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <memory>
#include <opencog/util/oc_assert.h>
#include "BaseServer.h"

using namespace opencog;

// The user might want to tell us about an existing AtomSpace.
BaseServer::BaseServer(AtomSpace* as)
{
    if (nullptr == as) {
        _atomSpace = new AtomSpace();
        _private_as = _atomSpace;
    }
    else {
        _atomSpace = as;
        _private_as = nullptr;
    }
}

BaseServer::~BaseServer()
{
    _atomSpace = nullptr;

    if (_private_as)
        delete _private_as;
}
