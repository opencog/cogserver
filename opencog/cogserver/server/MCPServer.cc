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
#include <opencog/cogserver/shell/McpEval.h>

using namespace opencog;

MCPServer::MCPServer(void)
{
	_eval = nullptr;
}

MCPServer::~MCPServer()
{
	logger().info("MCP Client disconnected");
}

// ==================================================================

// Called before any data is sent/received.
// We arrive here if MCP is NOT being started from a shell.
// This is in fact the usual or intended usage, as the only MCP shell
// users will be coders who are debugging stuff. But there's a catch:
// the shell automatically provides an evaluator. No shell means no
// evaluator, so we have to make one for ourself.
//
void MCPServer::OnConnection(void)
{
	logger().info("MCP Client connected");

	// If there's no shell, then set up an evaluator for ourself.
	if (nullptr == _shell)
		_eval = McpEval::get_evaluator(cogserver().getAtomSpace());
}

// Called for each newline-terminated line received.
void MCPServer::OnLine(const std::string& line)
{
	// If there's a shell, just use the shell evaluator.
	if (_shell)
	{
		_shell->eval(line);
		return;
	}

	// No shell? Do it ourself.
	_eval->begin_eval();
	_eval->eval_expr(line);
	Send(_eval->poll_result());
}

// ==================================================================
