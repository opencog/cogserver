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

#if HAVE_MCP
#include <nlohmann/json.hpp>
#endif // HAVE_MCP

#include <opencog/util/Logger.h>
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
		} else if (method == "initialized") {
			// Notification - no response
			_result = "";
			return;
		} else if (method == "ping") {
			response["result"] = json::object();
		} else if (method == "tools/list") {
			response["result"] = {
				{"tools", {
					{
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
					},
					{
						{"name", "time"},
						{"description", "Get current time"},
						{"inputSchema", {
							{"type", "object"},
							{"properties", json::object()}
						}}
					}
				}}
			};
		} else if (method == "resources/list") {
			response["result"] = {
				{"resources", json::array()}
			};
		} else if (method == "tools/call") {
			std::string tool_name = params.value("name", "");
			json arguments = params.value("arguments", json::object());

			if (tool_name == "echo") {
				std::string text = arguments.value("text", "");
				response["result"] = {
					{"content", {
						{
							{"type", "text"},
							{"text", "Echo: " + text}
						}
					}}
				};
			} else if (tool_name == "time") {
				auto now = std::chrono::system_clock::now();
				auto time_t = std::chrono::system_clock::to_time_t(now);
				response["result"] = {
					{"content", {
						{
							{"type", "text"},
							{"text", std::ctime(&time_t)}
						}
					}}
				};
			} else {
				response["error"] = {
					{"code", -32601},
					{"message", "Method not found: " + tool_name}
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

// One evaluator per thread.  This allows multiple users to each
// have thier own evaluator.
McpEval* McpEval::get_evaluator(const AtomSpacePtr& asp)
{
	static thread_local McpEval* evaluator = new McpEval(asp);

	// The eval_dtor runs when this thread is destroyed.
	class eval_dtor {
		public:
		~eval_dtor() { delete evaluator; }
	};
	static thread_local eval_dtor killer;

	return evaluator;
}

/* ===================== END OF FILE ======================== */
