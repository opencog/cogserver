/*
 * WebShellModule.cc
 *
 * Simple WebSockets shell
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
#include <opencog/cogserver/server/ServerConsole.h>

#include "WebShell.h"
#include "ShellModule.h"

using namespace opencog;

DEFINE_SHELL_MODULE(WebShellModule);
DECLARE_MODULE(WebShellModule);

WebShellModule::WebShellModule(CogServer& cs) : Module(cs)
{
}

void WebShellModule::init(void)
{
	_cogserver.registerRequest(shelloutRequest::info().id,
	                           &shelloutFactory);
}

WebShellModule::~WebShellModule()
{
	_cogserver.unregisterRequest(shelloutRequest::info().id);
}

bool WebShellModule::config(const char*)
{
	return false;
}

const RequestClassInfo&
WebShellModule::shelloutRequest::info(void)
{
	static const RequestClassInfo _cci("GET",
		"Enter the WebSockets shell",
		"Usage: GET /foo HTTP/1.1\n\n"
		"Enter the WebSockets JSON/Javascript interpreter shell. This shell\n"
		"provides a very minimal Javascript shell, with just enough functions\n"
		"to get Atoms and Values from an AtomSpace.\n\n"
		"It is used to provide a basic AtomSpace WebApp network server.\n"
		"Example usage: `AtomSpace.getAtoms(\"Node\", true)` will return all\n"
		"Nodes in the AtomSpace. For more info, see the README.md file at\n"
		"https://github.com/opencog/atomspace/tree/master/opencog/persist/json\n",
		true, false);
	return _cci;
}

/**
 * Register this shell with the console.
 */
bool
WebShellModule::shelloutRequest::execute(void)
{
	ServerConsole *con = this->get_console();
	OC_ASSERT(con, "Invalid Request object");

	WebShell *sh = new WebShell();
	sh->set_socket(con);
	sh->hush_prompt(true);
	send("yoo");

#if 0
	if (!_parameters.empty())
	{
		std::string &arg = _parameters.front();
		// if (arg == "quiet" || arg == "hush") hush = true;
	}
#endif

	return true;
}

/* ===================== END OF FILE ============================ */
