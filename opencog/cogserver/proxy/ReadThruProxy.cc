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
#include <opencog/persist/sexpr/Commands.h>
#include <opencog/persist/sexpr/Sexpr.h>

#include <opencog/cogserver/server/CogServer.h>
#include "ReadThruProxy.h"

using namespace opencog;

DECLARE_MODULE(ReadThruProxy);

ReadThruProxy::ReadThruProxy(CogServer& cs) : WriteThruProxy(cs)
{
}

void ReadThruProxy::init(void)
{
	WriteThruProxy::init();

	if (0 == _targets.size())
		logger().info("[Read-Thru Proxy] There aren't any targets to read from!");
}

ReadThruProxy::~ReadThruProxy()
{
}

bool ReadThruProxy::config(const char* cfg)
{
printf("duuuude read-thru proxy cfg %s\n", cfg);
	return false;
}

// TODO:
// * Need space-frame support!

void ReadThruProxy::setup(SexprEval* sev)
{
	WriteThruProxy::setup(sev);

	using namespace std::placeholders;  // for _1, _2, _3...

	// Install dispatch handlers.
	sev->install_handler("cog-incoming-by-type",
		std::bind(&ReadThruProxy::cog_incoming_by_type, this, _1));
}

// XXX FIXME? Tgis is a cut-n-paste of Commands::cog_incoming_by_type()
// from the main atomspace sexpr directory; just that, in the middle,
// we've added an extra step.
std::string ReadThruProxy::cog_incoming_by_type(const std::string& cmd)
{
	size_t pos = 0;
	Handle h = Sexpr::decode_atom(cmd, pos);
	pos++; // step past close-paren
	Type t = Sexpr::decode_type(cmd, pos);

	AtomSpacePtr as = _cogserver.getAtomSpace();
	h = as->add_atom(h);

	// Get all incoming sets from all targets.
	for (const StorageNodePtr& snp : _targets)
		snp->fetch_incoming_by_type(h, t);

	for (const StorageNodePtr& snp : _targets)
		snp->barrier();

	std::string alist = "(";
	for (const Handle& hi : h->getIncomingSetByType(t))
		alist += Sexpr::encode_atom(hi);

	alist += ")";
	return alist;
}

/* ===================== END OF FILE ============================ */
