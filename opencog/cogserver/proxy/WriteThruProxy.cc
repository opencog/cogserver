/*
 * opencog/cogserver/proxy/WriteThruProxy.cc
 *
 * Simple WriteThru proxy agent
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
#include "WriteThruProxy.h"

using namespace opencog;

DECLARE_MODULE(WriteThruProxy);

WriteThruProxy::WriteThruProxy(CogServer& cs) : Proxy(cs)
{
printf("duuuude write-thru proxy ctor\n");
}

void WriteThruProxy::init(void)
{
printf("duuuude write-thru proxy init\n");

	AtomSpace* as = &_cogserver.getAtomSpace();
	_truth_key = as->get_atom(
		createNode(PREDICATE_NODE, "*-TruthValueKey-*"));

	// Get all of the StorageNodes to which we will be
	// forwarding writes.
	HandleSeq hsns;
	as->get_handles_by_type(hsns, STORAGE_NODE, true);

	_targets.clear();
	for (const Handle& hsn : hsns)
	{
		StorageNodePtr snp = StorageNodeCast(hsn);

		// Check to see if the StorageNode is actually open;
		// We cannot write to nodes that are closed.
		if (snp->connected())
		{
			_targets.push_back(snp);
printf("duuude will write-thru to %s\n", snp->to_string().c_str());
		}

		// TODO: check if the StorageNode is read-only.
	}
}

WriteThruProxy::~WriteThruProxy()
{
}

bool WriteThruProxy::config(const char* cfg)
{
printf("duuuude write-thru proxy cfg %s\n", cfg);
	return false;
}

// TODO:
// * Need space-frame support!

void WriteThruProxy::setup(SexprEval* sev)
{
printf("duuuude proxy install stufffff!! %p\n", sev);

	// Read-only atomspace ... should check earlier!?
	AtomSpace* as = &_cogserver.getAtomSpace();
	if (as->get_read_only())
	{
		logger().info("Read-only atomspace; no write-through proxying!");
		return;
	}

	using namespace std::placeholders;  // for _1, _2, _3...

	// Install dispatch handlers.
#if 0
	sev->install_handler("cog-extract!",
		std::bind(&WriteThruProxy::cog_extract, this, _1));
	sev->install_handler("cog-extract-recursive!",
		std::bind(&WriteThruProxy::cog_extract_recursive, this, _1));

	sev->install_handler("cog-set-value!",
		std::bind(&WriteThruProxy::cog_set_value, this, _1));
	sev->install_handler("cog-set-values!",
		std::bind(&WriteThruProxy::cog_set_values, this, _1));
#endif
	sev->install_handler("cog-set-tv!",
		std::bind(&WriteThruProxy::cog_set_tv, this, _1));
}

std::string WriteThruProxy::cog_extract(const std::string& arg)
{
return "";
	//return Commands::cog_extract(arg);
}

std::string WriteThruProxy::cog_extract_recursive(const std::string& arg)
{
return "";
	//return Commands::cog_extract_recursive(arg);
}

std::string WriteThruProxy::cog_set_value(const std::string& arg)
{
return "";
	//return Commands::cog_set_value(arg);
}

std::string WriteThruProxy::cog_set_values(const std::string& arg)
{
return "";
	//return Commands::cog_set_values(arg);
}

std::string WriteThruProxy::cog_set_tv(const std::string& arg)
{
printf("duuude set tv %s\n", arg.c_str());
	size_t pos = 0;
	// Handle h = Sexpr::decode_atom(arg, pos, _space_map);
	Handle h = Sexpr::decode_atom(arg, pos);
	ValuePtr tv = Sexpr::decode_value(arg, ++pos);

	// Search for optional AtomSpace argument
	// AtomSpace* as = get_opt_as(arg, pos);
	AtomSpace* as = &_cogserver.getAtomSpace();

	Handle ha = as->add_atom(h);
	as->set_truthvalue(ha, TruthValueCast(tv));

	// Loop over all targets, and send them the new truth value.
	for (const StorageNodePtr& snp : _targets)
	{
		snp->store_value(ha, _truth_key);
	}

	return "()";
}

/* ===================== END OF FILE ============================ */
