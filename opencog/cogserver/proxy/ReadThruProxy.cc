/*
 * opencog/cogserver/proxy/ReadThruProxy.cc
 *
 * Simple ReadThru proxy agent
 * Copyright (c) 2008, 2020, 2022 Linas Vepstas <linas@linas.org>
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

#include <cstdio>

#include <opencog/atomspace/AtomSpace.h>
#include <opencog/atoms/base/Atom.h>
#include <opencog/persist/sexpr/Sexpr.h>

#include <opencog/cogserver/server/CogServer.h>
#include "ReadThruProxy.h"

using namespace opencog;
using namespace std::placeholders;  // for _1, _2, _3...

DECLARE_MODULE(ReadThruProxy);

ReadThruProxy::ReadThruProxy(CogServer& cs) : ThruProxy(cs)
{
}

void ReadThruProxy::init(void)
{
	ThruProxy::init();
}

ReadThruProxy::~ReadThruProxy()
{
}

bool ReadThruProxy::config(const char* cfg)
{
printf("duuuude read-thru proxy cfg %s\n", cfg);
	return false;
}


void ReadThruProxy::setup(SexprEval* sev)
{
	ThruProxy::setup(sev);

	// Install dispatch handlers.
	sev->install_handler("cog-incoming-by-type",
		std::bind(&ReadThruProxy::cog_incoming_by_type, this, _1));
}

// ------------------------------------------------------------------

std::string ReadThruProxy::cog_incoming_by_type(const std::string& arg)
{
	return _decoder.cog_incoming_by_type(arg,
		std::bind(&ReadThruProxy::incoming_by_type_cb, this, _1, _2));
}

std::string ReadThruProxy::cog_incoming_set(const std::string& arg)
{
	return _decoder.cog_incoming_set(arg,
		std::bind(&ReadThruProxy::incoming_set_cb, this, _1));
}

// ------------------------------------------------------------------
// The callbacks are called after the string command is decoded.

void ReadThruProxy::incoming_set_cb(const Handle& h)
{
	// Get all incoming sets from all targets.
	for (const StorageNodePtr& snp : _targets)
		snp->fetch_incoming_set(h);

	for (const StorageNodePtr& snp : _targets)
		snp->barrier();
}

void ReadThruProxy::incoming_by_type_cb(const Handle& h, Type t)
{
	// Get all incoming sets from all targets.
	for (const StorageNodePtr& snp : _targets)
		snp->fetch_incoming_by_type(h, t);

	for (const StorageNodePtr& snp : _targets)
		snp->barrier();
}

/* ===================== END OF FILE ============================ */
