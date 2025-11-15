/*
 * opencog/cogserver/server/MCPServer.h
 *
 * Copyright (C) 2025 Linas Vepstas
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef _OPENCOG_MCP_SERVER_H
#define _OPENCOG_MCP_SERVER_H

#include <string>

#include <opencog/network/ConsoleSocket.h>
#include <opencog/cogserver/server/CogServer.h>
#include <opencog/cogserver/mcp-eval/McpEval.h>

namespace opencog
{
/** \addtogroup grp_server
 *  @{
 */

/**
 * This class implements a simple Model Context Protocol MCP server.
 */
class MCPServer : public ConsoleSocket
{
private:
	CogServer& _cserver;
	McpEval* _eval;

protected:
	virtual void OnConnection(void);
	virtual void OnLine (const std::string&);

public:
    MCPServer(CogServer&, SocketManager*);
    ~MCPServer();

}; // class

/** @}*/
}  // namespace

#endif // _OPENCOG_MCP_SERVER_H
