/*
 * opencog/cogserver/proxy/ThruCommands.cc
 *
 * Simple Thru proxy agent
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
#include <opencog/persist/sexpr/Commands.h>
#include <opencog/persist/sexpr/Sexpr.h>

#include <opencog/cogserver/server/CogServer.h>
#include "ThruCommands.h"

using namespace opencog;

ThruCommands::ThruCommands() :
	_decoder(*dynamic_cast<UnwrappedCommands*>(this))
{
}

ThruCommands::~ThruCommands() {}

void ThruCommands::init(const AtomSpacePtr& asp)
{
	_as = asp;

	// Read-only atomspace ... maybe should check earlier!?
	if (_as->get_read_only())
	{
		logger().info("Read-only atomspace; no proxying!");
		return;
	}

	// Tell the command decoder what AtomSpace to use
	_decoder.set_base_space(_as);

	// Get all of the StorageNodes to which we will be
	// forwarding writes.
	HandleSeq hsns;
	_as->get_handles_by_type(hsns, STORAGE_NODE, true);

	_targets.clear();
	for (const Handle& hsn : hsns)
	{
		StorageNodePtr snp = StorageNodeCast(hsn);

		// Check to see if the StorageNode is actually open;
		// We cannot write to nodes that are closed.
		if (snp->connected())
		{
			_targets.push_back(snp);
			logger().info("[Thru Commands] Will pass-thru to %s\n",
				snp->to_short_string().c_str());
		}

		// TODO: check if the StorageNode is read-only.
	}

	if (0 == _targets.size())
		logger().info("[ThruCommands] There aren't any targets to work with!");
}

/* ===================== END OF FILE ============================ */
