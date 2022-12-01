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
#ifdef DEAD_CODE
	// At this time, the cfg is going to be a module name.
	// Get the module. If we don't have it, then load it.
	Module *ext = _cogserver.getModule(cfg);
	if (nullptr == ext) _cogserver.loadModule(cfg);

	ext = _cogserver.getModule(cfg);
	if (nullptr == ext)
	{
		logger().info("[SexprShell] unable to find config module %s", cfg);
		return false;
	}

	// Just record the config setting for now.
	// We should check it for valid syntax, and return false if it is
	// bad. ... but not ready for that, yet.
	_proxy_list.push_back(cfg);
#endif // DEAD_CODE
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

	SexprShell *sh = new SexprShell();

#ifdef DEAD_CODE
	// We don't do this, need this any more. Its here as
	// a coding example, I guess...
	for (const std::string& proxy: _proxy_list)
	{
		Module *ext = _cogserver.getModule(proxy);
		logger().info("[SexprShellModule] setup proxy: %p %s", ext, proxy.c_str());

		if (ext)
		{
			Proxy* pxy = dynamic_cast<Proxy*>(ext);
			OC_ASSERT(pxy, "Invalid Proxy object");

			GenericEval* gev = sh->get_evaluator();
			SexprEval* sev = dynamic_cast<SexprEval*>(gev);
			OC_ASSERT(sev, "Invalid SexprEval object");

			// Hand the shell over to the proxy for configuration.
			pxy->setup(sev);
		}
	}
#endif // DEAD_CODE

	sh->set_socket(con);
	send("");
	return true;
}

/* ===================== END OF FILE ============================ */
