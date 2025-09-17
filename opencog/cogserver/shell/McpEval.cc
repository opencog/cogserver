/**
 * McpEval.cc
 *
 * MCP protocol evaluator
 *
 * Copyright (c) 2008, 2014, 2015, 2020, 2021, 2022 Linas Vepstas
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
#include <algorithm>
#include <fstream>

#if HAVE_MCP
#include <json/json.h>
#include <sstream>
#endif // HAVE_MCP

#include <opencog/util/Logger.h>
#include <opencog/persist/json/McpPlugin.h>
#include <opencog/persist/json/McpPlugAtomSpace.h>
#include "McpPlugEcho.h"
#include "McpEval.h"

using namespace opencog;
using namespace std::chrono_literals;

McpEval::McpEval(const AtomSpacePtr& asp)
	: GenericEval(), _atomspace(asp)
{
	_started = false;
	_done = false;
}

McpEval::~McpEval()
{
}

/* ============================================================== */
/**
 * Evaluate MCP commands.
 */
void McpEval::eval_expr(const std::string &expr)
{
	if (0 == expr.size()) return;
	if (0 == expr.compare("\n")) return;

#if HAVE_MCP
	logger().info("[McpEval] received %s", expr.c_str());
	try
	{
		Json::Value request;
		Json::CharReaderBuilder reader_builder;
		std::unique_ptr<Json::CharReader> reader(reader_builder.newCharReader());
		std::string errors;

		if (!reader->parse(expr.c_str(), expr.c_str() + expr.length(), &request, &errors)) {
			Json::Value error_response;
			error_response["jsonrpc"] = "2.0";
			error_response["id"] = Json::Value::null;
			error_response["error"]["code"] = -32700;
			error_response["error"]["message"] = "Parse error: " + errors;
			_result = json_to_string(error_response) + "\n";
			return;
		}

		if (!request.isMember("jsonrpc") || request["jsonrpc"].asString() != "2.0")
		{
			Json::Value error_response;
			error_response["jsonrpc"] = "2.0";
			error_response["id"] = Json::Value::null;
			error_response["error"]["code"] = -32600;
			error_response["error"]["message"] = "Invalid Request - missing jsonrpc 2.0";
			_result = json_to_string(error_response) + "\n";
			return;
		}

		std::string method = request.isMember("method") ? request["method"].asString() : "";
		Json::Value params = request.isMember("params") ? request["params"] : Json::objectValue;
		Json::Value id = request.isMember("id") ? request["id"] : Json::Value::null;

		logger().debug("[McpEval] method %s", method.c_str());
		Json::Value response;
		response["jsonrpc"] = "2.0";
		response["id"] = id;

		if (method == "initialize") {
			// Use the latest MCP protocol version
			response["result"]["protocolVersion"] = "2025-06-18";

			// Indicate that we support tools by including the capability
			// The MCP spec states that presence of the key indicates support
			// An empty object {} means basic support without advanced features
			Json::Value toolsCapability;
			toolsCapability["listChanged"] = false;  // We don't support dynamic tool changes
			response["result"]["capabilities"]["tools"] = toolsCapability;

			// Indicate that we support resources for documentation
			Json::Value resourcesCapability;
			resourcesCapability["subscribe"] = false;  // No live updates for docs
			resourcesCapability["listChanged"] = false;  // Static resource list
			response["result"]["capabilities"]["resources"] = resourcesCapability;

			response["result"]["serverInfo"]["name"] = "CogServer MCP";
			response["result"]["serverInfo"]["version"] = "0.1.1";
			response["result"]["serverInfo"]["instructions"] =
				"The CogServer MCP provides access to a live, running instance of the "
				"AtomSpace. It allows the MCP client to view and manipulate the contents "
				"of the AtomSpace. This includes creating and deleting Atoms, changing "
				"the Values attached to them, running the executable Atoms, and sending "
            "messages to those Atoms that implement an Object interface.";
		} else if (method == "notifications/initialized" or
		           method == "initialized") {
#define WTF_INIT
#ifdef WTF_INIT
			// Supposedly there should be no response here, but its
			// broken if we're silent. So ... !???
			response["result"] = Json::objectValue;
#else
			// Notification - no response
			_result = "";
			_done = true;
			return;
#endif
		} else if (method == "notifications/cancelled") {
			// XXX FIXME .. the currently running tool should be killed.
			// To do this, the Mcpplugin API would need to get a new
			// kill method that would get called to do this. Later.
			// This method is supposed to not have any response.
			_result = "";
			_done = true;
			return;
		} else if (method == "ping") {
			response["result"] = Json::objectValue;
		} else if (method == "tools/list") {
			Json::Value all_tools(Json::arrayValue);

			// Collect tools from all registered plugins
			for (const auto& plugin : _plugins) {
				std::string plugin_tools_json = plugin->get_tool_descriptions();
				Json::Value plugin_tools;
				Json::CharReaderBuilder builder;
				std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
				std::string errors;

				if (reader->parse(plugin_tools_json.c_str(),
				                  plugin_tools_json.c_str() + plugin_tools_json.length(),
				                  &plugin_tools, &errors)) {
					for (Json::ArrayIndex i = 0; i < plugin_tools.size(); ++i) {
						all_tools.append(plugin_tools[i]);
					}
				}
			}

			response["result"]["tools"] = all_tools;
		} else if (method == "resources/list") {
			Json::Value resources(Json::arrayValue);

			// Add the AtomSpace introduction document (shorter overview)
			Json::Value intro_resource;
			intro_resource["uri"] = "atomspace://docs/introduction";
			intro_resource["name"] = "AtomSpace Introduction";
			intro_resource["description"] = "Overview of the AtomSpace, Atoms, and basic concepts";
			intro_resource["mimeType"] = "text/markdown";
			resources.append(intro_resource);

			// Add the detailed AtomSpace guide (longer detailed guide)
			Json::Value guide_resource;
			guide_resource["uri"] = "atomspace://docs/atomspace-guide";
			guide_resource["name"] = "AtomSpace Detailed Guide";
			guide_resource["description"] = "Comprehensive guide to Atomese, the AtomSpace, and the CogServer";
			guide_resource["mimeType"] = "text/markdown";
			resources.append(guide_resource);

			response["result"]["resources"] = resources;
		} else if (method == "resources/read") {
			std::string uri = params.isMember("uri") ? params["uri"].asString() : "";

			// Use the CMAKE install prefix for the documentation path
			std::string doc_base = std::string(PROJECT_INSTALL_PREFIX) + "/share/cogserver/mcp/";

			if (uri == "atomspace://docs/introduction") {
				// Read the AtomSpace-Overview.md file (shorter introduction)
				std::string doc_path = doc_base + "AtomSpace-Overview.md";
				std::ifstream file(doc_path);
				if (file.is_open()) {
					std::stringstream buffer;
					buffer << file.rdbuf();
					file.close();

					Json::Value content;
					content["uri"] = uri;
					content["mimeType"] = "text/markdown";
					content["text"] = buffer.str();
					response["result"]["contents"] = Json::arrayValue;
					response["result"]["contents"].append(content);
				} else {
					response["error"]["code"] = -32602;
					response["error"]["message"] = "Failed to read documentation file: " + doc_path;
				}
			} else if (uri == "atomspace://docs/atomspace-guide") {
				// Read the AtomSpace-Details.md file (longer detailed guide)
				std::string doc_path = doc_base + "AtomSpace-Details.md";
				std::ifstream file(doc_path);
				if (file.is_open()) {
					std::stringstream buffer;
					buffer << file.rdbuf();
					file.close();

					Json::Value content;
					content["uri"] = uri;
					content["mimeType"] = "text/markdown";
					content["text"] = buffer.str();
					response["result"]["contents"] = Json::arrayValue;
					response["result"]["contents"].append(content);
				} else {
					response["error"]["code"] = -32602;
					response["error"]["message"] = "Failed to read documentation file: " + doc_path;
				}
			} else {
				response["error"]["code"] = -32602;
				response["error"]["message"] = "Resource not found: " + uri;
			}
		} else if (method == "tools/call") {
			std::string tool_name = params.isMember("name") ? params["name"].asString() : "";
			Json::Value arguments = params.isMember("arguments") ? params["arguments"] : Json::objectValue;

			// Find the plugin that handles this tool
			auto it = _tool_to_plugin.find(tool_name);
			if (it != _tool_to_plugin.end()) {
				// Invoke the tool through the plugin
				std::string arguments_json = json_to_string(arguments);
				std::string tool_result_json = it->second->invoke_tool(tool_name, arguments_json);

				Json::Value tool_result;
				Json::CharReaderBuilder builder;
				std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
				std::string errors;

				if (reader->parse(tool_result_json.c_str(),
				                  tool_result_json.c_str() + tool_result_json.length(),
				                  &tool_result, &errors)) {
					// Check if the plugin returned an error
					if (tool_result.isMember("error")) {
						response["error"] = tool_result["error"];
					} else {
						response["result"] = tool_result;
					}
				} else {
					response["error"]["code"] = -32700;
					response["error"]["message"] = "Failed to parse tool result";
				}
			} else {
				response["error"]["code"] = -32601;
				response["error"]["message"] = "Tool not found: " + tool_name;
			}
		} else {
			response["error"]["code"] = -32601;
			response["error"]["message"] = "Method not found: " + method;
		}

		logger().info("[McpEval] replying: %s", json_to_string(response).c_str());

		// Trailing newline is mandatory; jsonrpc uses line discipline.
		_result = json_to_string(response) + "\n";
		_done = true;
	}
	catch (const std::exception& e)
	{
		Json::Value error_response;
		error_response["jsonrpc"] = "2.0";
		error_response["id"] = Json::Value::null;
		error_response["error"]["code"] = -32700;
		error_response["error"]["message"] = "Parse error: " + std::string(e.what());
		logger().info("[McpEval] error reply: %s", json_to_string(error_response).c_str());
		_result = json_to_string(error_response) + "\n";
		_done = true;
	}
#else
	_result = "MCP support not compiled in\n";
	_done = true;
#endif //HAVE_MCP
}

std::string McpEval::poll_result()
{
	if (_done) {
		_done = false;
		return _result;
	}

	return "";
}

void McpEval::begin_eval()
{
	_done = false;
}

/* ============================================================== */

/**
 * interrupt() - convert user's control-C at keyboard into exception.
 */
void McpEval::interrupt(void)
{
	_done = true;
	_started = false;
	_caught_error = true;
}

/* ============================================================== */

/**
 * Register a plugin to provide MCP tools
 */
void McpEval::register_plugin(std::shared_ptr<McpPlugin> plugin)
{
#if HAVE_MCP
	if (!plugin) return;

	_plugins.push_back(plugin);

	// Map each tool to its plugin
	std::string tools_json = plugin->get_tool_descriptions();
	Json::Value tools;
	Json::CharReaderBuilder builder;
	std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	std::string errors;

	if (reader->parse(tools_json.c_str(), tools_json.c_str() + tools_json.length(), &tools, &errors)) {
		for (Json::ArrayIndex i = 0; i < tools.size(); ++i) {
			std::string tool_name = tools[i]["name"].asString();
			_tool_to_plugin[tool_name] = plugin;
		}
	}
#endif // HAVE_MCP
}

/**
 * Unregister a plugin
 */
void McpEval::unregister_plugin(std::shared_ptr<McpPlugin> plugin)
{
#if HAVE_MCP
	if (!plugin) return;

	// Remove from plugins list
	_plugins.erase(
		std::remove(_plugins.begin(), _plugins.end(), plugin),
		_plugins.end()
	);

	// Remove tool mappings
	auto it = _tool_to_plugin.begin();
	while (it != _tool_to_plugin.end()) {
		if (it->second == plugin) {
			it = _tool_to_plugin.erase(it);
		} else {
			++it;
		}
	}
#endif // HAVE_MCP
}

/* ============================================================== */

// One evaluator per thread.  This allows multiple users to each
// have their own evaluator.
McpEval* McpEval::get_evaluator(const AtomSpacePtr& asp)
{
	static thread_local McpEval* evaluator = new McpEval(asp);

	// Install the plugins here, for now. XXX Hack alert, this is
	// not the right way to do this long-term, but it holds water for
	// now.
	auto echo_plugin = std::make_shared<McpPlugEcho>();
	evaluator->register_plugin(echo_plugin);
	auto as_plugin = std::make_shared<McpPlugAtomSpace>(asp.get());
	evaluator->register_plugin(as_plugin);

	// The eval_dtor runs when this thread is destroyed.
	class eval_dtor {
		public:
		~eval_dtor() { delete evaluator; }
	};
	static thread_local eval_dtor killer;

	return evaluator;
}

/* ===================== END OF FILE ======================== */
