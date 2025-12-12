/*
 * opencog/cogserver/atoms/CogServerNode.cc
 *
 * Copyright (c) 2025 BrainyBlaze Dynamics, LLC
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
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 */

#include <unordered_set>

#include <opencog/atoms/base/ClassServer.h>
#include <opencog/atoms/core/NumberNode.h>
#include "CogServerNode.h"

using namespace opencog;

/// Implement Jenkins' One-at-a-Time hash for message dispatch.
static constexpr uint32_t dispatch_hash(const char* s)
{
	uint32_t hash = 0;

	for(; *s; ++s)
	{
		hash += *s;
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}

	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);

	return hash;
}

CogServerNode::CogServerNode(Type t, const std::string&& s)
	: Node(t, std::move(s)), CogServer()
{
}

CogServerNode::CogServerNode(const std::string&& s)
	: Node(COG_SERVER_NODE, std::move(s)), CogServer()
{
}

AtomSpacePtr CogServerNode::getAS()
{
	return AtomSpaceCast(_atom_space);
}

HandleSeq CogServerNode::getMessages() const
{
	static const HandleSeq msgs = []() {
		HandleSeq m({
			createNode(PREDICATE_NODE, "*-start-*"),
			createNode(PREDICATE_NODE, "*-stop-*")
		});
		// Mark each message predicate as a message, once at load time.
		for (const Handle& h : m)
			h->markIsMessage();
		return m;
	}();

	// Copy list above into the local AtomSpace.
	HandleSeq lms;
	for (const Handle& m : msgs)
		lms.emplace_back(_atom_space->add_atom(m));
	return lms;
}

bool CogServerNode::usesMessage(const Handle& key) const
{
	static const std::unordered_set<uint32_t> msgset({
		dispatch_hash("*-start-*"),
		dispatch_hash("*-stop-*")
	});

	if (PREDICATE_NODE != key->get_type()) return false;

	const std::string& pred = key->get_name();
	uint32_t dhsh = dispatch_hash(pred.c_str());
	if (msgset.find(dhsh) != msgset.end()) return true;
	return false;
}

void CogServerNode::setValue(const Handle& key, const ValuePtr& value)
{
	// If not a PredicateNode, just store the value.
	if (PREDICATE_NODE != key->get_type())
	{
		Atom::setValue(key, value);
		return;
	}

	// Create a fast dispatch table by using case-statement
	// branching, instead of string compare.
	static constexpr uint32_t p_start = dispatch_hash("*-start-*");
	static constexpr uint32_t p_stop = dispatch_hash("*-stop-*");

	const std::string& pred = key->get_name();
	switch (dispatch_hash(pred.c_str()))
	{
		case p_start:
		{
			// The value should be a NumberNode holding the port number.
			int port = 17001;
			if (value and value->is_type(NUMBER_NODE))
				port = NumberNodeCast(value)->get_value();
			enableNetworkServer(port);
			return;
		}
		case p_stop:
			stop();
			return;
		default:
			break;
	}

	// Some other predicate. Store it.
	Atom::setValue(key, value);
}

DEFINE_NODE_FACTORY(CogServerNode, COG_SERVER_NODE)
