/**
 * McpPlugEcho.cc
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

#include <chrono>
#include <ctime>

#if HAVE_MCP
#include <nlohmann/json.hpp>
#endif

#include "McpPlugEcho.h"

using namespace opencog;

std::string McpPlugEcho::get_tool_descriptions() const
{
#if HAVE_MCP
    using namespace nlohmann;
    json tools = json::array();

    // Echo tool description
    tools.push_back({
        {"name", "echo"},
        {"description", "Echo the input text"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", {
                {"text", {
                    {"type", "string"},
                    {"description", "Text to echo"}
                }}
            }},
            {"required", {"text"}}
        }}
    });

    // Time tool description
    tools.push_back({
        {"name", "time"},
        {"description", "Get current time"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", json::object()}
        }}
    });

    return tools.dump();
#else
    return "[]";
#endif
}

std::string McpPlugEcho::invoke_tool(const std::string& tool_name,
                                     const std::string& arguments) const
{
#if HAVE_MCP
    using namespace nlohmann;
    json response;

    try {
        if (tool_name == "echo") {
            json args = json::parse(arguments);
            std::string text = args.value("text", "");
            response["content"] = {
                {
                    {"type", "text"},
                    {"text", "Echo: " + text}
                }
            };
        } else if (tool_name == "time") {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            response["content"] = {
                {
                    {"type", "text"},
                    {"text", std::ctime(&time_t)}
                }
            };
        } else {
            // Tool not found in this plugin
            response["error"] = {
                {"code", -32601},
                {"message", "Tool not found in McpPlugEcho: " + tool_name}
            };
        }
    } catch (const json::parse_error& e) {
        response["error"] = {
            {"code", -32700},
            {"message", "Parse error: " + std::string(e.what())}
        };
    }

    return response.dump();
#else
    return "{\"error\":{\"code\":-32601,\"message\":\"MCP not compiled\"}}";
#endif
}
