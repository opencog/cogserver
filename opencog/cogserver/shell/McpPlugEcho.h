/*
 * McpPlugEcho.h
 *
 * MCP plugin providing echo and time tools
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

#ifndef _OPENCOG_MCP_PLUG_ECHO_H
#define _OPENCOG_MCP_PLUG_ECHO_H

#include <opencog/persist/json/McpPlugin.h>

namespace opencog {

/**
 * McpPlugEcho - A plugin providing basic echo and time tools
 */
class McpPlugEcho : public McpPlugin
{
public:
    McpPlugEcho() = default;
    virtual ~McpPlugEcho() = default;

    /**
     * Get the list of tool descriptions provided by this plugin.
     * Returns descriptions for:
     * - echo: Echo the input text
     * - time: Get current time
     */
    virtual std::string get_tool_descriptions() const override;

    /**
     * Invoke a tool provided by this plugin.
     * Handles:
     * - echo: Returns the input text prefixed with "Echo: "
     * - time: Returns the current system time
     */
    virtual std::string invoke_tool(const std::string& tool_name,
                                   const std::string& arguments) const override;
};

} // namespace opencog

#endif // _OPENCOG_MCP_PLUG_ECHO_H
