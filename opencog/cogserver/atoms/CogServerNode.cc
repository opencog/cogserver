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

#include <opencog/util/Logger.h>
#include <opencog/atoms/base/ClassServer.h>
#include <opencog/atoms/core/NumberNode.h>
#include <opencog/atoms/value/FloatValue.h>
#include <opencog/atoms/value/StringValue.h>
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
	CogServer::loadModules();
}

CogServerNode::CogServerNode(const std::string&& s)
	: Node(COG_SERVER_NODE, std::move(s)), CogServer()
{
	CogServer::loadModules();
}

void CogServerNode::setAtomSpace(AtomSpace* as)
{
	Node::setAtomSpace(as);
	if (nullptr == as) return;

	// Port defaults
	Atom::setValue(as->add_node(PREDICATE_NODE, "*-telnet-port-*"),
	               createFloatValue(17001.0));
	Atom::setValue(as->add_node(PREDICATE_NODE, "*-web-port-*"),
	               createFloatValue(18080.0));
	Atom::setValue(as->add_node(PREDICATE_NODE, "*-mcp-port-*"),
	               createFloatValue(18888.0));

	// Prompt defaults
	Atom::setValue(as->add_node(PREDICATE_NODE, "*-ansi-prompt-*"),
	               createStringValue("[0;32mopencog[1;32m> [0m"));
	Atom::setValue(as->add_node(PREDICATE_NODE, "*-prompt-*"),
	               createStringValue("opencog> "));
	Atom::setValue(as->add_node(PREDICATE_NODE, "*-ansi-scm-prompt-*"),
	               createStringValue("[0;34mguile[1;34m> [0m"));
	Atom::setValue(as->add_node(PREDICATE_NODE, "*-scm-prompt-*"),
	               createStringValue("guile> "));
}

/// Retrieve a port number from a stored Value.
/// Accepts FloatValue or NumberNode.
int CogServerNode::getPortValue(const char* key, int defaultPort)
{
	Handle hkey = createNode(PREDICATE_NODE, key);
	ValuePtr vp = Atom::getValue(hkey);
	if (nullptr == vp) return defaultPort;

	if (vp->is_type(FLOAT_VALUE))
		return FloatValueCast(vp)->value()[0];

	if (vp->is_type(NUMBER_NODE))
		return NumberNodeCast(vp)->get_value();

	return defaultPort;
}

std::string CogServerNode::getStringValue(const char* key,
                                          const std::string& defaultVal)
{
	Handle hkey = createNode(PREDICATE_NODE, key);
	ValuePtr vp = Atom::getValue(hkey);
	if (nullptr == vp) return defaultVal;

	if (vp->is_type(STRING_VALUE))
		return StringValueCast(vp)->value()[0];

	if (vp->is_node())
		return HandleCast(vp)->get_name();

	return defaultVal;
}

/// Start the servers
void CogServerNode::startServers()
{
	int telnet_port = getPortValue("*-telnet-port-*", 17001);
	int web_port = getPortValue("*-web-port-*", 18080);
	int mcp_port = getPortValue("*-mcp-port-*", 18888);


	// This try-catch block is a historical appendage,
	// it is here to try to protect us against ASIO insanity.
	// I think this "never happens any more", but never say never.
	try
	{
		if (0 < telnet_port)
			enableNetworkServer(telnet_port);
		if (0 < web_port)
			enableWebServer(web_port);
		if (0 < mcp_port)
			enableMCPServer(mcp_port);
	}
	catch (const std::exception& ex)
	{
		logger().error("Failed to start server: %s", ex.what());
	}

	_main_loop = new std::thread(&CogServerNode::serverLoop, this);
}

void CogServerNode::stopServers()
{
	CogServer::stop();
	if (_main_loop)
	{
		_main_loop->join();
		delete _main_loop;
		_main_loop = nullptr;
	}
	disableMCPServer();
	disableWebServer();
	disableNetworkServer();
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
			startServers();
			return;
		case p_stop:
			stopServers();
			return;
		default:
			break;
	}

	// Some other predicate. Store it.
	Atom::setValue(key, value);
}

DEFINE_NODE_FACTORY(CogServerNode, COG_SERVER_NODE)
