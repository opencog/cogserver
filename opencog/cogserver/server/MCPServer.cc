/*
 * opencog/cogserver/server/MCPServer.cc
 *
 * Copyright (C) 2025 Linas Vepstas
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <cstddef>
#include <string>

#include <opencog/util/exceptions.h>
#include <opencog/util/Logger.h>

#include <opencog/cogserver/server/CogServer.h>
#include <opencog/cogserver/server/MCPServer.h>

using namespace opencog;

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
#if 0
	if (_request)
	{
		// Use the request mechanism to get a fully configured
		// shell. This is a hang-over from the telnet interfaces,
		// where input strings become Requests, which, when executed
		// are looked up in the module system, passed to the correct
		// module, and then configured to send replies on this socket.
		// It works, so don't mess with it.
		std::list<std::string> params;
		params.push_back("hush");
		_request->setParameters(params);
		_request->set_console(this);
		_request->execute();
		delete _request;
		_request = nullptr;

		// Disable line discipline
		// _shell->discipline(false);
	}
#endif
	_shell->eval(line);
}

// ==================================================================
