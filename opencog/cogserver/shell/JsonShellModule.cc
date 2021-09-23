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
#include <opencog/cogserver/server/ServerConsole.h>

#include "JsonShell.h"
#include "JsonShellModule.h"

using namespace opencog;

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

const RequestClassInfo&
JsonShellModule::shelloutRequest::info(void)
{
	static const RequestClassInfo _cci("json",
		"Enter the JSON shell",
		"Usage: json\n\n"
		"Enter the JSON/Javascript interpreter shell. This shell provides\n"
		"a very minimal Javascript shell, with just enough functions to get\n"
		"Atoms and Values from an AtomSpace.\n\n"
		"It is used to provide a basic AtomSpace WebApp network server.\n"
		"Example usage: `AtomSpace.getAtoms(\"Node\", true)` will return all\n"
		"Nodes in the AtomSpace.\n\n"
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
	ServerConsole *con = this->get_console();
	OC_ASSERT(con, "Invalid Request object");

	JsonShell *sh = new JsonShell();
	sh->set_socket(con);

	send("");
	return true;
}

/* ===================== END OF FILE ============================ */
