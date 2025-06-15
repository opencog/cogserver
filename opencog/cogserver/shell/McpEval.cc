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

#if HAVE_MCP
#include <nlohmann/json.hpp>
#endif // HAVE_MCP

#include <opencog/util/Logger.h>
#include <opencog/persist/json/McpPlugin.h>
#include <opencog/persist/json/McpPlugAtomSpace.h>
#include "McpPlugEcho.h"
#include "McpEval.h"

using namespace opencog;
using namespace std::chrono_literals;
#if HAVE_MCP
using namespace nlohmann;
#endif // HAVE_MCP

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
		json request = json::parse(expr);
		if (!request.contains("jsonrpc") || request["jsonrpc"] != "2.0")
		{
			json error_response;
			error_response["jsonrpc"] = "2.0";
			error_response["id"] = json();
			error_response["error"] = {
				{"code", -32600},
				{"message", "Invalid Request - missing jsonrpc 2.0"}
			};
			_result = error_response.dump() + "\n";
			return;
		}

		std::string method = request.value("method", "");
		json params = request.value("params", json::object());
		json id = request.value("id", json());

		logger().debug("[McpEval] method %s", method.c_str());
		json response;
		response["jsonrpc"] = "2.0";
		response["id"] = id;

		if (method == "initialize") {
			response["result"] = {
				{"protocolVersion", "2024-11-05"},
				{"capabilities", {
					{"tools", json::object()},
					{"resources", json::object()}
				}},
				{"serverInfo", {
					{"name", "CogServer MCP"},
					{"version", "0.1.0"}
				}}
			};
		} else if (method == "notifications/initialized" or
		           method == "initialized") {
#define WTF_INIT
#ifdef WTF_INIT
			// Supposedly there should be no response here, but its
			// broken if we're silent. So ... !???
			response["result"] = json::object();
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
			response["result"] = json::object();
		} else if (method == "tools/list") {
			json all_tools = json::array();

			// Collect tools from all registered plugins
			for (const auto& plugin : _plugins) {
				std::string plugin_tools_json = plugin->get_tool_descriptions();
				json plugin_tools = json::parse(plugin_tools_json);
				for (const auto& tool : plugin_tools) {
					all_tools.push_back(tool);
				}
			}

			response["result"] = {
				{"tools", all_tools}
			};
		} else if (method == "resources/list") {
			response["result"] = {
				{"resources", json::array()}
			};
		} else if (method == "tools/call") {
			std::string tool_name = params.value("name", "");
			json arguments = params.value("arguments", json::object());

			// Find the plugin that handles this tool
			auto it = _tool_to_plugin.find(tool_name);
			if (it != _tool_to_plugin.end()) {
				// Invoke the tool through the plugin
				std::string arguments_json = arguments.dump();
				std::string tool_result_json = it->second->invoke_tool(tool_name, arguments_json);
				json tool_result = json::parse(tool_result_json);

				// Check if the plugin returned an error
				if (tool_result.contains("error")) {
					response["error"] = tool_result["error"];
				} else {
					response["result"] = tool_result;
				}
			} else {
				response["error"] = {
					{"code", -32601},
					{"message", "Tool not found: " + tool_name}
				};
			}
		} else {
			response["error"] = {
				{"code", -32601},
				{"message", "Method not found: " + method}
			};
		}

		logger().info("[McpEval] replying: %s", response.dump().c_str());

		// Trailing newline is mandatory; jsonrpc uses line discipline.
		_result = response.dump() + "\n";
		_done = true;
	}
	catch (const std::exception& e)
	{
		json error_response;
		error_response["jsonrpc"] = "2.0";
		error_response["id"] = json();
		error_response["error"] = {
			{"code", -32700},
			{"message", "Parse error: " + std::string(e.what())}
		};
		logger().info("[McpEval] error reply: %s", error_response.dump().c_str());
		_result = error_response.dump() + "\n";
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
	json tools = json::parse(tools_json);
	for (const auto& tool : tools) {
		std::string tool_name = tool["name"];
		_tool_to_plugin[tool_name] = plugin;
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
// have thier own evaluator.
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
