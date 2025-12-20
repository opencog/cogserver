/*
 * opencog/cogserver/atoms/CogServerNode.h
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

#ifndef _OPENCOG_COG_SERVER_NODE_H
#define _OPENCOG_COG_SERVER_NODE_H

#include <opencog/atoms/base/Node.h>
#include <opencog/atomspace/AtomSpace.h>
#include <opencog/cogserver/server/CogServer.h>
#include <opencog/cogserver/types/atom_types.h>

#include <thread>

namespace opencog
{
/** \addtogroup grp_atomspace
 *  @{
 *
 * CogServerNode wraps a CogServer instance. The name of the node
 * provides a unique identifier for the server. Values can be attached
 * to this node to hold server configuration and state.
 */

class CogServerNode : public Node, public CogServer
{
public:
	// Please do NOT use this constructor!
	CogServerNode(Type, const std::string&&);

public:
	CogServerNode(const std::string&&);

	CogServerNode(CogServerNode&) = delete;
	CogServerNode& operator=(const CogServerNode&) = delete;

	virtual void setValue(const Handle&, const ValuePtr&);
	virtual ValuePtr getValue(const Handle&) const;
	virtual void setAtomSpace(AtomSpace*) override;
	virtual HandleSeq getMessages() const;
	virtual bool usesMessage(const Handle&) const;

	Handle getHandle() override { return get_handle(); }

	static Handle factory(const Handle&);

private:
	bool startServers();
	void stopServers();
	std::thread* _main_loop = nullptr;
};

NODE_PTR_DECL(CogServerNode)
#define createCogServerNode CREATE_DECL(CogServerNode)

/** @}*/
}

#endif // _OPENCOG_COG_SERVER_NODE_H
