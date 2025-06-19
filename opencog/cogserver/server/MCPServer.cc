/*
 * opencog/cogserver/server/MCPServer.cc
 *
 * Copyright (C) 2025 Linas Vepstas
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifdef HAVE_MCP

#include <cstddef>
#include <string>
#include <cctype>

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
#ifdef ONE_JSON_OBJECT_PER_LINE
	_eval->begin_eval();
	_eval->eval_expr(buffer.substr(start));
	Send(_eval->poll_result());
	return;
#endif // ONE_JSON_OBJECT_PER_LINE

	// The above assumes one JSON object per line, which is a reasonable
	// assumption for the base design. However, the MCP proxying breaks
	// this assumption, so the code below loops, handing off each object
	// that it finds. Its ... necessarily slow, because it passes over
	// all the data cahracter by character, and then does some string
	// copies. So taht's pretty dumb. For for now, I simply don't care.
	// If you need a high-speed interface, use the sexpr API, not the
	// JSON API.

	// Handle multiple JSON objects on a single line
	std::string buffer = line;
	size_t pos = 0;

	while (pos < buffer.length())
	{
		// Skip whitespace
		while (pos < buffer.length() && std::isspace(buffer[pos]))
			pos++;

		if (pos >= buffer.length())
			break;

		// Find the end of this JSON object by counting braces
		size_t start = pos;
		int brace_count = 0;
		bool in_string = false;
		bool escape_next = false;

		while (pos < buffer.length())
		{
			char c = buffer[pos];

			if (!escape_next && c == '\\')
			{
				escape_next = true;
			}
			else if (!escape_next && c == '"')
			{
				in_string = !in_string;
			}
			else if (!in_string && !escape_next)
			{
				if (c == '{')
					brace_count++;
				else if (c == '}')
				{
					brace_count--;
					if (brace_count == 0)
					{
						pos++;
						break;
					}
				}
			}
			else
			{
				escape_next = false;
			}

			pos++;
		}

		// Extract and process this JSON object
		if (brace_count == 0 && pos > start)
		{
			std::string json_obj = buffer.substr(start, pos - start);
			_eval->begin_eval();
			_eval->eval_expr(json_obj);
			Send(_eval->poll_result());
		}
		else
		{
			// Malformed JSON, process the whole remaining string
			_eval->begin_eval();
			_eval->eval_expr(buffer.substr(start));
			Send(_eval->poll_result());
			return;
		}
	}
}

#endif // HAVE_MCP
// ==================================================================
