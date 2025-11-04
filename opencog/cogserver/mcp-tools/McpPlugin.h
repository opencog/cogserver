/*
 * McpPlugin.h
 *
 * Base class for MCP tool plugins
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

#ifndef _OPENCOG_MCP_PLUGIN_H
#define _OPENCOG_MCP_PLUGIN_H

#include <string>
#include <vector>

namespace opencog {

/**
 * Base class defining an API for MCP-style JSON plugins.
 *
 * The CogServer uses this class to manage plugins defining MCP tools.
 * MCP is the Model Context Protocol; it is an experimental feature in
 * the CogServer. However, the API here is sufficiently generic that it
 * can work for JSON APIs in general. Thus, it is here as an
 * "mcp-inspired" interface that might be generically useful.
 */
class McpPlugin
{
public:
	virtual ~McpPlugin() = default;

	/**
	 * Get the list of tool descriptions provided by this plugin.
	 * Returns a JSON string containing an array of tool description
	 * objects. Each tool description should have:
	 * - name: string
	 * - description: string
	 * - inputSchema: JSON schema object
	 */
	virtual std::string get_tool_descriptions() const = 0;

	/**
	 * Invoke a tool provided by this plugin.
	 * @param tool_name The name of the tool to invoke
	 * @param arguments JSON string containing the arguments for the tool
	 * @return JSON string containing the response with result or error
	 */
	virtual std::string invoke_tool(const std::string& tool_name,
	                                const std::string& arguments) const = 0;
};

} // namespace opencog

#endif // _OPENCOG_MCP_PLUGIN_H
