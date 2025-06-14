/*
 * McpPlugin.h
 *
 * Base class for MCP tool plugins
 * Copyright (c) 2024 OpenCog Foundation
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

#if HAVE_MCP
#include <nlohmann/json.hpp>
#endif

namespace opencog {

/**
 * Base class for MCP tool plugins.
 * Plugins implementing this interface can provide a set of MCP tools
 * that can be registered with McpEval.
 */
class McpPlugin
{
public:
    virtual ~McpPlugin() = default;

#if HAVE_MCP
    /**
     * Get the list of tool descriptions provided by this plugin.
     * Returns a JSON array of tool description objects.
     * Each tool description should have:
     * - name: string
     * - description: string
     * - inputSchema: JSON schema object
     */
    virtual nlohmann::json get_tool_descriptions() const = 0;

    /**
     * Invoke a tool provided by this plugin.
     * @param tool_name The name of the tool to invoke
     * @param arguments The JSON arguments for the tool
     * @return JSON response with result or error
     */
    virtual nlohmann::json invoke_tool(const std::string& tool_name, 
                                      const nlohmann::json& arguments) const = 0;
#endif // HAVE_MCP
};

} // namespace opencog

#endif // _OPENCOG_MCP_PLUGIN_H