/*
 * opencog/cogserver/server/MCPServer.cc
 *
 * Copyright (C) 2025 Linas Vepstas
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <string>

#include <opencog/util/exceptions.h>
#include <opencog/util/Logger.h>
#include <opencog/util/misc.h>

#include <opencog/cogserver/server/CogServer.h>
#include <opencog/cogserver/server/MCPServer.h>

using namespace opencog;

MCPServer::MCPServer(void) :
	_request(nullptr)
{
}

MCPServer::~MCPServer()
{
	logger().info("Closed MCP Server");
}

// ==================================================================

// Called before any data is sent/received.
void MCPServer::OnConnection(void)
{
	logger().info("Opened MCP Shell");
}

// Called for each newline-terminated line received.
void MCPServer::OnLine(const std::string& line)
{
}

// ==================================================================
