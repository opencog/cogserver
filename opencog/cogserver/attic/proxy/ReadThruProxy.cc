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

DECLARE_MODULE(ReadThruProxy);

ReadThruProxy::ReadThruProxy(CogServer& cs) : Proxy(cs) {}

void ReadThruProxy::init(void) {}

ReadThruProxy::~ReadThruProxy() {}

bool ReadThruProxy::config(const char* cfg)
{
printf("duuuude read-thru proxy cfg %s\n", cfg);
	return false;
}

void ReadThruProxy::setup(SexprEval* sev)
{
   _rthru_wrap.init(_cogserver.getAtomSpace());
   _rthru_wrap.setup(sev);
}

// ------------------------------------------------------------------
// The callbacks are called after the string command is decoded.

ReadThru::ReadThru(void)
{
	have_get_atoms_cb = true;
	have_incoming_set_cb = true;
	have_incoming_by_type_cb = true;
	have_keys_alist_cb = true;
	have_node_cb = true;
	have_link_cb = true;
	have_value_cb = true;
}

ReadThru::~ReadThru() {}

void ReadThru::setup(SexprEval* sev)
{
	using namespace std::placeholders;  // for _1, _2, _3...

	// Install dispatch handlers.
#define INST(STR,CB) \
	sev->install_handler(STR, std::bind(&Commands::CB, &_decoder, _1));

	INST("cog-get-atoms",        cog_get_atoms);
	INST("cog-incoming-by-type", cog_incoming_by_type);
	INST("cog-incoming-set",     cog_incoming_set);
	INST("cog-keys->alist",      cog_keys_alist);
	INST("cog-link",             cog_link);
	INST("cog-node",             cog_node);
	INST("cog-value",            cog_value);
}

// ------------------------------------------------------------------

void ReadThru::get_atoms_cb(Type type, bool get_subtypes)
{
	// Get all atoms from all targets.
	for (const StorageNodePtr& snp : _targets)
	{
		snp->fetch_all_atoms_of_type(type);
		if (get_subtypes)
		{
			for (Type t = type+1; t < nameserver().getNumberOfClasses(); t++)
			{
				if (nameserver().isA(t, type))
					snp->fetch_all_atoms_of_type(t);
			}
		}
	}

	for (const StorageNodePtr& snp : _targets)
		snp->barrier();
}

void ReadThru::incoming_set_cb(const Handle& h)
{
	// Get all incoming sets from all targets.
	for (const StorageNodePtr& snp : _targets)
		snp->fetch_incoming_set(h);

	for (const StorageNodePtr& snp : _targets)
		snp->barrier();
}

void ReadThru::incoming_by_type_cb(const Handle& h, Type t)
{
	// Get all incoming sets from all targets.
	for (const StorageNodePtr& snp : _targets)
		snp->fetch_incoming_by_type(h, t);

	for (const StorageNodePtr& snp : _targets)
		snp->barrier();
}

void ReadThru::keys_alist_cb(const Handle& h)
{
	// Get all keys from all targets.
	for (const StorageNodePtr& snp : _targets)
	{
		snp->fetch_atom(h);
		snp->barrier();
	}
}

void ReadThru::node_cb(const Handle& h)
{
	// Get the node from all targets; last one wins.
	for (const StorageNodePtr& snp : _targets)
	{
		snp->fetch_atom(h);
		snp->barrier();
	}
}

void ReadThru::link_cb(const Handle& h)
{
	// Get the link from all targets; last one wins.
	for (const StorageNodePtr& snp : _targets)
	{
		snp->fetch_atom(h);
		snp->barrier();
	}
}

void ReadThru::value_cb(const Handle& atom, const Handle& key)
{
	// Get the value from all targets; last one wins.
	for (const StorageNodePtr& snp : _targets)
	{
		snp->fetch_value(atom, key);
		snp->barrier();
	}
}

/* ===================== END OF FILE ============================ */
