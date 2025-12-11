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

#include <opencog/atoms/base/ClassServer.h>
#include "CogServerNode.h"

using namespace opencog;

CogServerNode::CogServerNode(Type t, const std::string&& s)
	: Node(t, std::move(s)), CogServer()
{
}

CogServerNode::CogServerNode(const std::string&& s)
	: Node(COG_SERVER_NODE, std::move(s)), CogServer()
{
}

void CogServerNode::setValue(const Handle& key, const ValuePtr& value)
{
	// Call the base class implementation
	Atom::setValue(key, value);
}

DEFINE_NODE_FACTORY(CogServerNode, COG_SERVER_NODE)
