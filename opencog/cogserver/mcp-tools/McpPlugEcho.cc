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
#include <json/json.h>
#include <sstream>
#endif

#include "McpPlugEcho.h"

using namespace opencog;

std::string McpPlugEcho::get_tool_descriptions() const
{
#if HAVE_MCP
    Json::Value tools(Json::arrayValue);

    // Echo tool description
    Json::Value echo_tool;
    echo_tool["name"] = "echo";
    echo_tool["description"] = "Echo the input text";
    echo_tool["inputSchema"]["type"] = "object";
    echo_tool["inputSchema"]["properties"]["text"]["type"] = "string";
    echo_tool["inputSchema"]["properties"]["text"]["description"] = "Text to echo";
    echo_tool["inputSchema"]["required"].append("text");
    tools.append(echo_tool);

    // Time tool description
    Json::Value time_tool;
    time_tool["name"] = "time";
    time_tool["description"] = "Get current time";
    time_tool["inputSchema"]["type"] = "object";
    time_tool["inputSchema"]["properties"] = Json::objectValue;
    tools.append(time_tool);

    return json_to_string(tools);
#else
    return "[]";
#endif
}

std::string McpPlugEcho::invoke_tool(const std::string& tool_name,
                                     const std::string& arguments) const
{
#if HAVE_MCP
    Json::Value response;

    try {
        if (tool_name == "echo") {
            Json::Value args;
            Json::CharReaderBuilder builder;
            std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
            std::string errors;

            if (reader->parse(arguments.c_str(), arguments.c_str() + arguments.length(), &args, &errors)) {
                std::string text = args.isMember("text") ? args["text"].asString() : "";
                Json::Value content_item;
                content_item["type"] = "text";
                content_item["text"] = "Echo: " + text;
                response["content"].append(content_item);
            } else {
                response["error"]["code"] = -32700;
                response["error"]["message"] = "Parse error: " + errors;
            }
        } else if (tool_name == "time") {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            Json::Value content_item;
            content_item["type"] = "text";
            content_item["text"] = std::ctime(&time_t);
            response["content"].append(content_item);
        } else {
            // Tool not found in this plugin
            response["error"]["code"] = -32601;
            response["error"]["message"] = "Tool not found in McpPlugEcho: " + tool_name;
        }
    } catch (const std::exception& e) {
        response["error"]["code"] = -32700;
        response["error"]["message"] = "Parse error: " + std::string(e.what());
    }

    return json_to_string(response);
#else
    return "{\"error\":{\"code\":-32601,\"message\":\"MCP not compiled\"}}";
#endif
}
