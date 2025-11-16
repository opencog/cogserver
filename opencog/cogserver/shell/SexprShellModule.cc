/*
 * SexprShellModule.cc
 *
 * Simple s-expression shell
 * Copyright (c) 2008, 2020 Linas Vepstas <linas@linas.org>
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
#include <opencog/network/SocketManager.h>
#include <opencog/persist/sexcom/SexprEval.h>

#include "SexprShell.h"
#include "ShellModule.h"

using namespace opencog;

DEFINE_SHELL_MODULE(SexprShellModule);
DECLARE_MODULE(SexprShellModule);

SexprShellModule::SexprShellModule(CogServer& cs) : Module(cs)
{
}

void SexprShellModule::init(void)
{
	_cogserver.registerRequest(shelloutRequest::info().id,
	                           &shelloutFactory);
}

SexprShellModule::~SexprShellModule()
{
	_cogserver.unregisterRequest(shelloutRequest::info().id);
}

// This is currently unused.
bool SexprShellModule::config(const char* cfg)
{
	return true;
}

const RequestClassInfo&
SexprShellModule::shelloutRequest::info(void)
{
	static const RequestClassInfo _cci("sexpr",
		"Enter the s-expression shell",
		"Usage: sexpr\n\n"
		"Enter the s-expression interpreter shell. This shell provides\n"
		"a very minimal s-expression shell, with just enough commands\n"
		"to interpret Atomese strings and move Atoms and Values between\n"
		"AtomSpaces.\n\n"
		"It is used to provide the basic AtomSpace network server.  It is\n"
		"not intended for manual use!  There is minimal error reporting\n"
		"and user-accessible help.  The commands are processed by\n"
		"https://github.com/opencog/atomspace/tree/master/opencog/persist/sexpr/Commands.cc\n"
		"See that file for details. Example usage: `(cog-get-atoms 'Node #t)`\n"
		"will return a list of all Nodes in the AtomSpace.\n\n"
		"Use either a ^D (ctrl-D) or a single . on a line by itself to exit\n"
		"the shell.\n\n",
		true, false);
	return _cci;
}

/**
 * Register this shell with the console.
 */
bool
SexprShellModule::shelloutRequest::execute(void)
{
	ConsoleSocket *con = this->get_console();
	OC_ASSERT(con, "Invalid Request object");

	SexprShell *sh = new SexprShell(_cogserver.getAtomSpace());

	// Install cog-barrier handler
	SexprEval* eval = dynamic_cast<SexprEval*>(sh->get_evaluator());
	eval->install_handler("cog-barrier)",
		[con](const std::string&) -> std::string {
			con->get_socket_manager()->barrier();
			return "";
		});

	sh->set_socket(con);
	send("");
	return true;
}

/* ===================== END OF FILE ============================ */
