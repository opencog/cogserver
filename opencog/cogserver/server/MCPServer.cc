/*
 * opencog/cogserver/server/MCPServer.cc
 *
 * Copyright (C) 2025 Linas Vepstas
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <cstddef>
#include <string>

#if HAVE_MCP
#include <mcp/json.hpp>
#endif // HAVE_MCP

#include <opencog/util/exceptions.h>
#include <opencog/util/Logger.h>

#include <opencog/cogserver/server/CogServer.h>
#include <opencog/cogserver/server/MCPServer.h>

using namespace opencog;
#if HAVE_MCP
using namespace nlohmann;
#endif // HAVE_MCP

MCPServer::MCPServer(void)
{
}

MCPServer::~MCPServer()
{
	logger().info("MCP Client disconnected");
}

// ==================================================================

// Called before any data is sent/received.
void MCPServer::OnConnection(void)
{
	logger().info("MCP Client connected");
}

// Called for each newline-terminated line received.
void MCPServer::OnLine(const std::string& line)
{
#if HAVE_MCP
	logger().debug("[MCPServer] received %s", line.c_str());
	try
	{
		json request = json::parse(line);
		if (!request.contains("jsonrpc") || request["jsonrpc"] != "2.0")
			return; // Invalid JSON-RPC

		std::string method = request.value("method", "");
		json params = request.value("params", json::object());
		json id = request.value("id", json());

		logger().debug("[MCPServer] method %s", method.c_str());
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
			return;
		} else if (method == "ping") {
			response["result"] = json::object();
		}

		logger().debug("[MCPServer] replying: %s", response.dump().c_str());
		// Trailing newline is mandatory; jsonrpc uses line discipline.
		Send(response.dump() + "\n");
		return;
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
		Send(error_response.dump() + "\n");
	}
#endif //HAVE_MCP
}

// ==================================================================
