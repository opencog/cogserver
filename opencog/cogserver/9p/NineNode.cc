/*
 * opencog/cogserver/9p/NineNode.cc
 *
 * Copyright (c) 2024 OpenCog Foundation
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <opencog/cogserver/9p/NineNode.h>

namespace opencog
{
namespace plan9
{

NineNodePtr NineNode::lookup(const std::string& child_name) const
{
	for (const auto& c : children())
		if (c and c->name() == child_name)
			return c;
	return nullptr;
}

} // namespace plan9
} // namespace opencog
