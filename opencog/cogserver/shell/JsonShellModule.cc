/*
 * JsonShellModule.cc
 *
 * Simple JSON shell
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

#include "JsonShell.h"
#include "ShellModule.h"

using namespace opencog;

DEFINE_SHELL_MODULE(JsonShellModule);
DECLARE_MODULE(JsonShellModule);

JsonShellModule::JsonShellModule(CogServer& cs) : Module(cs)
{
}

void JsonShellModule::init(void)
{
	_cogserver.registerRequest(shelloutRequest::info().id,
	                           &shelloutFactory);
}

JsonShellModule::~JsonShellModule()
{
	_cogserver.unregisterRequest(shelloutRequest::info().id);
}

bool JsonShellModule::config(const char*)
{
	return false;
}

const RequestClassInfo&
JsonShellModule::shelloutRequest::info(void)
{
	static const RequestClassInfo _cci("json",
		"Enter the JSON shell",
		"Usage: json [hush] [quiet]\n\n"
		"Enter the JSON/Javascript interpreter shell. This shell provides\n"
		"a very minimal Javascript shell, with just enough functions to get\n"
		"Atoms and Values from an AtomSpace.\n\n"
		"It is used to provide a basic AtomSpace WebApp network server.\n"
		"Example usage: `AtomSpace.getAtoms(\"Node\", true)` will return all\n"
		"Nodes in the AtomSpace. For more info, see the README.md file at\n"
		"https://github.com/opencog/atomspace/tree/master/opencog/persist/json\n\n"
		"By default, this prints a prompt. To get a shell without a prompt,\n"
		"say `json hush` or `json quiet`\n"
		"To exit the shell, send a ^D (ctrl-D) or a single . on a line by itself.\n",
		true, false);
	return _cci;
}

/**
 * Register this shell with the console.
 */
bool
JsonShellModule::shelloutRequest::execute(void)
{
	ConsoleSocket *con = this->get_console();
	OC_ASSERT(con, "Invalid Request object");

	JsonShell *sh = new JsonShell();
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
		"Entering JSON shell; use ^D or a single . on a "
		"line by itself to exit.\n" + sh->get_prompt();
	send(rv);
	return true;
}

/* ===================== END OF FILE ============================ */
