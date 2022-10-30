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
#include <functional>

#include <opencog/atomspace/AtomSpace.h>
#include <opencog/atoms/base/Atom.h>
#include <opencog/persist/sexpr/Sexpr.h>

#include <opencog/cogserver/server/CogServer.h>
#include "WriteThruProxy.h"

using namespace opencog;

DECLARE_MODULE(WriteThruProxy);

WriteThruProxy::WriteThruProxy(CogServer& cs) : Proxy(cs) {}

void WriteThruProxy::init(void) {}

WriteThruProxy::~WriteThruProxy() {}

bool WriteThruProxy::config(const char* cfg)
{
printf("duuuude write-thru proxy cfg %s\n", cfg);
	return false;
}

void WriteThruProxy::setup(SexprEval* sev)
{
	_wthru_wrap.init(_cogserver.getAtomSpace());
	_wthru_wrap.setup(sev);
}

// ------------------------------------------------------------------
// The callbacks are called after the string command is decoded.

WriteThru::WriteThru(void)
{
	have_extract_cb = true;
	have_extract_recursive_cb = true;
	have_set_value_cb = true;
	have_set_values_cb = true;
	have_set_tv_cb = true;
	have_update_value_cb = true;
}

WriteThru::~WriteThru() {}

void WriteThru::setup(SexprEval* sev)
{
	using namespace std::placeholders;  // for _1, _2, _3...

	// Install dispatch handlers.
	sev->install_handler("cog-extract!",
		std::bind(&Commands::cog_extract, &_decoder, _1));
	sev->install_handler("cog-extract-recursive!",
		std::bind(&Commands::cog_extract_recursive, &_decoder, _1));

	sev->install_handler("cog-set-value!",
		std::bind(&Commands::cog_set_value, &_decoder, _1));
	sev->install_handler("cog-set-values!",
		std::bind(&Commands::cog_set_values, &_decoder, _1));
	sev->install_handler("cog-set-tv!",
		std::bind(&Commands::cog_set_tv, &_decoder, _1));
	sev->install_handler("cog-update-value!",
		std::bind(&Commands::cog_update_value, &_decoder, _1));
}

// ------------------------------------------------------------------

void WriteThru::extract_cb(const Handle& h, bool flag)
{
	// Loop over all targets, and extract there as well.
	for (const StorageNodePtr& snp : _targets)
		snp->remove_atom(_as, h, flag);
}

void WriteThru::set_value_cb(const Handle& atom, const Handle& key,
                                  const ValuePtr& v)
{
	// Loop over all targets, and send them the new value.
	for (const StorageNodePtr& snp : _targets)
		snp->store_value(atom, key);
}

void WriteThru::set_values_cb(const Handle& atom)
{
	// Loop over all targets, and store everything.
	// In principle, we should be selective, and only pass
	// on the values we were given... this would require
	// Sexpr::decode_slist to return a list of keys,
	// and then we'd have to store one key at a time,
	// which seems inefficient. But still ... XXX FIXME ?
	for (const StorageNodePtr& snp : _targets)
		snp->store_atom(atom);
}

void WriteThru::set_tv_cb(const Handle& ha, const TruthValuePtr& tv)
{
	// Make sure we can store truth values!
	if (nullptr == _truth_key)
		_truth_key = _as->add_atom(
			createNode(PREDICATE_NODE, "*-TruthValueKey-*"));

	// Loop over all targets, and send them the new truth value.
	for (const StorageNodePtr& snp : _targets)
		snp->store_value(ha, _truth_key);
}

void WriteThru::update_value_cb(const Handle& atom, const Handle& key,
                                     const ValuePtr& delta)
{
	// Loop over all targets, and send them the delta value.
	for (const StorageNodePtr& snp : _targets)
		snp->update_value(atom, key, delta);
}

/* ===================== END OF FILE ============================ */
