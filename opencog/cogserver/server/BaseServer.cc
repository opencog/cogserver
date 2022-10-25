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
BaseServer::BaseServer(void)
{
	_atomSpace = createAtomSpace();
}

BaseServer::BaseServer(AtomSpacePtr as)
{
	if (nullptr == as)
		_atomSpace = createAtomSpace();
	else
		_atomSpace = as;
}

BaseServer::~BaseServer()
{
	_atomSpace = nullptr;
}
