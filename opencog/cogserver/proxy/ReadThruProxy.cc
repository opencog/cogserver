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
	sev->install_handler("cog-node",
		std::bind(&ReadThruProxy::cog_node, this, _1));
}

std::string ReadThruProxy::cog_node(const std::string& arg)
{
#if 0
	size_t pos = 0;
	Type t = Sexpr::decode_type(cmd, pos);

	size_t l = pos+1;
	size_t r = cmd.size();
	std::string name = Sexpr::get_node_name(cmd, l, r, t);

	AtomSpacePtr as = _cogserver.getAtomSpace();

	// Return copy from the first target that finds the node.
	for (const StorageNodePtr& snp : _targets)
	{
		snp->tom(as, h, );
		return Sexpr::encode_atom(h, _multi_space);
	}
this is wrong
#endif

	return "()";
}

/* ===================== END OF FILE ============================ */
