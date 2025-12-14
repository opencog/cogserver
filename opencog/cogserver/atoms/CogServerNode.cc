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
#include <opencog/atoms/atom_types/atom_names.h>
#include <opencog/atoms/base/ClassServer.h>
#include <opencog/atoms/value/BoolValue.h>
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
}

CogServerNode::CogServerNode(const std::string&& s)
	: Node(COG_SERVER_NODE, std::move(s)), CogServer()
{
}

void CogServerNode::setAtomSpace(AtomSpace* as)
{
	Node::setAtomSpace(as);
	if (nullptr == as) return;

	// Port defaults
	Atom::setValue(as->add_atom(Predicate("*-telnet-port-*")),
	               createFloatValue(17001.0));
	Atom::setValue(as->add_atom(Predicate("*-web-port-*")),
	               createFloatValue(18080.0));
	Atom::setValue(as->add_atom(Predicate("*-mcp-port-*")),
	               createFloatValue(18888.0));

	// Prompt defaults
	Atom::setValue(as->add_atom(Predicate("*-ansi-prompt-*")),
	               createStringValue("\033[0;32mopencog\033[1;32m> \033[0m"));
	Atom::setValue(as->add_atom(Predicate("*-prompt-*")),
	               createStringValue("opencog> "));
	Atom::setValue(as->add_atom(Predicate("*-ansi-scm-prompt-*")),
	               createStringValue("\033[0;34mguile\033[1;34m> \033[0m"));
	Atom::setValue(as->add_atom(Predicate("*-scm-prompt-*")),
	               createStringValue("guile> "));

	// ANSI color enable/disable
	Atom::setValue(as->add_atom(Predicate("*-ansi-enabled-*")),
	               createBoolValue(true));

	CogServer::loadModules(get_handle());
}

/// Enable the network servers.
/// Returns true if the servers were started, false if already running.
bool CogServerNode::startServers()
{
	if (CogServer::set_running()) return false;

	// This try-catch block is a historical appendage,
	// it is here to try to protect us against ASIO insanity.
	// I think this "never happens any more", but never say never.
	try
	{
		Handle h(get_handle());
		enableNetworkServer(h);
		enableWebServer(h);
		enableMCPServer(h);
	}
	catch (const std::exception& ex)
	{
		logger().error("Failed to start server: %s", ex.what());
	}
	return true;
}

/// Disable the network servers
void CogServerNode::stopServers()
{
	disableMCPServer();
	disableWebServer();
	disableNetworkServer();
}

HandleSeq CogServerNode::getMessages() const
{
	static const HandleSeq msgs = []() {
		HandleSeq m({
			Predicate("*-start-*"),
			Predicate("*-stop-*"),
			Predicate("*-run-*"),
			Predicate("*-is-running?-*")
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
		dispatch_hash("*-stop-*"),
		dispatch_hash("*-run-*"),
		dispatch_hash("*-is-running?-*")
	});

	if (PREDICATE_NODE != key->get_type()) return false;

	const std::string& pred = key->get_name();
	uint32_t dhsh = dispatch_hash(pred.c_str());
	if (msgset.find(dhsh) != msgset.end()) return true;
	return false;
}

ValuePtr CogServerNode::getValue(const Handle& key) const
{
	// Handle the *-is-running?-* query
	if (PREDICATE_NODE == key->get_type())
	{
		static constexpr uint32_t p_is_running = dispatch_hash("*-is-running?-*");
		const std::string& pred = key->get_name();
		if (dispatch_hash(pred.c_str()) == p_is_running)
			return createBoolValue(CogServer::running());
	}

	// Pass through to base class for all other keys
	return Atom::getValue(key);
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
	static constexpr uint32_t p_run = dispatch_hash("*-run-*");

	const std::string& pred = key->get_name();
	switch (dispatch_hash(pred.c_str()))
	{
		case p_start:
			if (not startServers()) return;
			_main_loop = new std::thread([this]() { serverLoop(); });
			return;

		case p_stop:
			if (not CogServer::running()) return;
			CogServer::stop();
			if (_main_loop)
			{
				_main_loop->join();
				delete _main_loop;
				_main_loop = nullptr;
			}
			stopServers();
			return;

		case p_run:
			startServers();
			serverLoop();
			stopServers();
			return;

		default:
			break;
	}

	// Some other predicate. Store it.
	Atom::setValue(key, value);
}

DEFINE_NODE_FACTORY(CogServerNode, COG_SERVER_NODE)

// Init function for guile load-extension. The factory registration
// happens via static constructors in DEFINE_NODE_FACTORY above.
extern "C" void opencog_servernode_init(void) {}
