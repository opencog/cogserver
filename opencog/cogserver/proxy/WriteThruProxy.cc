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
using namespace std::placeholders;  // for _1, _2, _3...

DECLARE_MODULE(WriteThruProxy);

WriteThruProxy::WriteThruProxy(CogServer& cs) : ThruProxy(cs)
{
}

void WriteThruProxy::init(void)
{
	ThruProxy::init();
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
	ThruProxy::setup(sev);

	// Read-only atomspace ... should check earlier!?
	if (_as->get_read_only())
	{
		logger().info("Read-only atomspace; no write-through proxying!");
		return;
	}

	// Install dispatch handlers.
	sev->install_handler("cog-extract!",
		std::bind(&WriteThruProxy::cog_extract, this, _1));
	sev->install_handler("cog-extract-recursive!",
		std::bind(&WriteThruProxy::cog_extract_recursive, this, _1));

	sev->install_handler("cog-set-value!",
		std::bind(&WriteThruProxy::cog_set_value, this, _1));
	sev->install_handler("cog-set-values!",
		std::bind(&WriteThruProxy::cog_set_values, this, _1));
	sev->install_handler("cog-set-tv!",
		std::bind(&WriteThruProxy::cog_set_tv, this, _1));
	sev->install_handler("cog-update-value!",
		std::bind(&WriteThruProxy::cog_update_value, this, _1));
}

// ------------------------------------------------------------------

std::string WriteThruProxy::cog_extract(const std::string& arg)
{
	return _decoder.cog_extract(arg,
		std::bind(&WriteThruProxy::extract_cb, this, _1, false));
}

std::string WriteThruProxy::cog_extract_recursive(const std::string& arg)
{
	return _decoder.cog_extract_recursive(arg,
		std::bind(&WriteThruProxy::extract_cb, this, _1, true));
}

std::string WriteThruProxy::cog_set_value(const std::string& arg)
{
	return _decoder.cog_set_value(arg,
		std::bind(&WriteThruProxy::set_value_cb, this, _1, _2, _3));
}

std::string WriteThruProxy::cog_set_values(const std::string& arg)
{
	return _decoder.cog_set_values(arg,
		std::bind(&WriteThruProxy::set_values_cb, this, _1));
}

std::string WriteThruProxy::cog_set_tv(const std::string& arg)
{
	return _decoder.cog_set_tv(arg,
		std::bind(&WriteThruProxy::set_tv_cb, this, _1, _2));
}

std::string WriteThruProxy::cog_update_value(const std::string& arg)
{
	return _decoder.cog_update_value(arg,
		std::bind(&WriteThruProxy::update_value_cb, this, _1, _2, _3));
}

// ------------------------------------------------------------------
// The callbacks are called after the string command is decoded.

void WriteThruProxy::extract_cb(const Handle& h, bool flag)
{
	// Loop over all targets, and extract there as well.
	for (const StorageNodePtr& snp : _targets)
		snp->remove_atom(_as, h, flag);
}

void WriteThruProxy::set_value_cb(const Handle& atom, const Handle& key,
                                  const ValuePtr& v)
{
	// Loop over all targets, and send them the new value.
	for (const StorageNodePtr& snp : _targets)
		snp->store_value(atom, key);
}

void WriteThruProxy::set_values_cb(const Handle& atom)
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

void WriteThruProxy::set_tv_cb(const Handle& ha, const TruthValuePtr& tv)
{
	// Make sure we can store truth values!
	if (nullptr == _truth_key)
		_truth_key = _as->add_atom(
			createNode(PREDICATE_NODE, "*-TruthValueKey-*"));

	// Loop over all targets, and send them the new truth value.
	for (const StorageNodePtr& snp : _targets)
		snp->store_value(ha, _truth_key);
}

void WriteThruProxy::update_value_cb(const Handle& atom, const Handle& key,
                                     const ValuePtr& delta)
{
	// Loop over all targets, and send them the delta value.
	for (const StorageNodePtr& snp : _targets)
		snp->update_value(atom, key, delta);
}

/* ===================== END OF FILE ============================ */
