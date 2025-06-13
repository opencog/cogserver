/*
 * McpShellModule.cc
 *
 * Simple MCP shell
 * Copyright (c) 2008, 2020, 2021 Linas Vepstas <linas@linas.org>
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

#include <opencog/util/Logger.h>
#include <opencog/util/oc_assert.h>
#include <opencog/cogserver/server/CogServer.h>
#include <opencog/cogserver/server/Module.h>
#include <opencog/cogserver/server/Request.h>
#include <opencog/network/ConsoleSocket.h>

#include "McpShell.h"
#include "ShellModule.h"

using namespace opencog;

DEFINE_SHELL_MODULE(McpShellModule);
DECLARE_MODULE(McpShellModule);

McpShellModule::McpShellModule(CogServer& cs) : Module(cs)
{
}

void McpShellModule::init(void)
{
	_cogserver.registerRequest(shelloutRequest::info().id,
	                           &shelloutFactory);
}

McpShellModule::~McpShellModule()
{
	_cogserver.unregisterRequest(shelloutRequest::info().id);
}

bool McpShellModule::config(const char*)
{
	return false;
}

const RequestClassInfo&
McpShellModule::shelloutRequest::info(void)
{
	static const RequestClassInfo _cci("mcp",
		"Enter the MCP shell",
		"Usage: mcp [hush] [quiet]\n\n"
		"Enter the Model Context Protocol (MCP) shell. This shell provides\n"
		"a shell that implements MCP. It goes through the same MCP code as\n"
		"provided at the CogServer MCP port, the only difference being that\n"
		"this might be easier to use when debugging the MCP implementation\n"
		"by hand.\n\n"
		"Example usage: `{\"methods\":\"tools/list\",\"jsonrpc\":\"2.0\",\"id\":1}`\n"
		"will return all MCP tools currently avaiable.\n"
		"By default, this prints a prompt. To get a shell without a prompt,\n"
		"say `mcp hush` or `mcp quiet`\n"
		"To exit the shell, send a ^D (ctrl-D) or a single . on a line by itself.\n",
		true, false);
	return _cci;
}

/**
 * Register this shell with the console.
 */
bool
McpShellModule::shelloutRequest::execute(void)
{
	ConsoleSocket *con = this->get_console();
	OC_ASSERT(con, "Invalid Request object");

	McpShell *sh = new McpShell();
	sh->set_socket(con);

	if (!_parameters.empty())
	{
		bool hush = false;
		std::string &arg = _parameters.front();
		if (arg == "quiet" || arg == "hush") hush = true;
		sh->hush_prompt(hush);

		if (hush) { send(""); return true; }
	}

	std::string rv =
		"Entering MCP shell; use ^D or a single . on a "
		"line by itself to exit.\n" + sh->get_prompt();
	send(rv);
	return true;
}

/* ===================== END OF FILE ============================ */
