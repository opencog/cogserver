/*
 * opencog/cogserver/server/BaseServer.cc
 *
 * Copyright (C) 2002-2007 Novamente LLC
 * All Rights Reserved
 *
 * Written by Andre Senna <senna@vettalabs.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License v3 as
 * published by the Free Software Foundation and including the exceptions
 * at http://opencog.org/wiki/Licenses
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, write to:
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <memory>
#include <opencog/util/oc_assert.h>
#include "BaseServer.h"

using namespace opencog;

AtomSpace* BaseServer::_atomSpace = nullptr;

// There might already be an atomspace, whose management we should
// take over.  The user can specify this atomspace.
BaseServer::BaseServer(AtomSpace* as)
{
    // We shouldn't get called with a non-NULL atomSpace static global;
    // that's indicative of a missing call to BaseServer::~BaseServer.
    if (_atomSpace) {
        throw (RuntimeException(TRACE_INFO,
               "Reinitialized BaseServer!"));
    }

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

AtomSpace& BaseServer::getAtomSpace()
{
    return *_atomSpace;
}
