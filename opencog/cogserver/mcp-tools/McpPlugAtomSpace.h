/*
 * McpPlugAtomSpace.h
 *
 * MCP Plugin for AtomSpace operations
 * Copyright (c) 2025 Linas Vepstas
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

#ifndef _OPENCOG_MCP_PLUG_ATOMSPACE_H
#define _OPENCOG_MCP_PLUG_ATOMSPACE_H

#include <opencog/atomspace/AtomSpace.h>
#include "McpPlugin.h"

namespace opencog {

/**
 * MCP Plugin providing AtomSpace operations as MCP tools.
 * 
 * This plugin exposes the core AtomSpace functionality through the MCP
 * protocol, allowing clients to create, query, and manipulate atoms
 * using JSON-formatted requests.
 */
class McpPlugAtomSpace : public McpPlugin
{
private:
	AtomSpace* _as;

public:
	McpPlugAtomSpace(AtomSpace* as) : _as(as) {}
	virtual ~McpPlugAtomSpace() = default;

	/**
	 * Get the list of AtomSpace tools provided by this plugin.
	 */
	virtual std::string get_tool_descriptions() const;

	/**
	 * Invoke an AtomSpace tool.
	 * @param tool_name The name of the tool to invoke
	 * @param arguments JSON string containing the arguments for the tool
	 * @return JSON string containing the response with result or error
	 */
	virtual std::string invoke_tool(const std::string& tool_name,
	                                const std::string& arguments) const;
};

} // namespace opencog

#endif // _OPENCOG_MCP_PLUG_ATOMSPACE_H
