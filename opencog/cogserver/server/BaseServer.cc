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

static BaseServer* serverInstance = nullptr;

BaseServer* BaseServer::createInstance(AtomSpace* as)
{
    OC_ASSERT(0,
        "Accidentally called the base class!\n"
        "To fix this bug, be sure to make the following call in your code:\n"
        "   server(MyServer::MyCreateInstance);\n"
        "maybe this:\n"
        "   server(CogServer::createInstance);\n"
        "depending on which you want.  You only need to do this once,\n"
        "early during the initialization of your program.\n"
    );
    return NULL;
}


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

    // Set this server as the current server.
    if (serverInstance)
        throw (RuntimeException(TRACE_INFO,
                "Can't create more than one server singleton instance!"));

    serverInstance = this;
}

BaseServer::~BaseServer()
{
    serverInstance = nullptr;
    _atomSpace = nullptr;

    if (_private_as)
        delete _private_as;
}

AtomSpace& BaseServer::getAtomSpace()
{
    return *_atomSpace;
}

BaseServer& opencog::server(BaseServer* (*factoryFunction)(AtomSpace*),
                            AtomSpace* as)
{
    // Create a new instance using the factory function if we don't
    // already have one.
    if (!serverInstance)
        serverInstance = (*factoryFunction)(as);
    
    // Return a reference to our server instance.
    return *serverInstance;
}
