/*
 * SchemeShell.cc
 *
 * Simple scheme shell
 * Copyright (c) 2008 Linas Vepstas <linas@linas.org>
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

#ifdef HAVE_GUILE

#include <opencog/util/Config.h>
#include <opencog/util/Logger.h>
#include <opencog/guile/SchemeEval.h>
#include <opencog/cogserver/server/CogServer.h>

#include "SchemeShell.h"

using namespace opencog;

std::string SchemeShell::_prompt;

SchemeShell::SchemeShell(const AtomSpacePtr& asp) :
	_shellspace(asp)
{
	_prompt = "[0;34mguile[1;34m> [0m";

	// Avoid crash in cogserver dtor.
	if (nullptr != &config())
	{
		// Prompt with ANSI color codes, if possible.
		if (config().get_bool("ANSI_ENABLED", true))
			_prompt = config().get("ANSI_SCM_PROMPT", "[0;34mguile[1;34m> [0m");
		else
			_prompt = config().get("SCM_PROMPT", "guile> ");
	}

	normal_prompt = _prompt;
	abort_prompt = _prompt;
	pending_prompt = "... ";
	_name = " scm";

	// Set the inital atomspace for this thread.
	SchemeEval::set_scheme_as(_shellspace.get());
}

SchemeShell::~SchemeShell()
{
	// We must stall until after the evaluator has finished
	// evaluating. Otherwise, the thread_init() method (below)
	// might never get a chance to run, leading to a NULL
	// atomspace, leading to a crash.  Bug #2328.
	while_not_done();
}

GenericEval* SchemeShell::get_evaluator(void)
{
	return SchemeEval::get_evaluator(_shellspace);
}

/**
 * Register this shell with the console.
 */
void SchemeShell::thread_init(void)
{
	SchemeEval::set_scheme_as(_shellspace.get());
}

#endif
/* ===================== END OF FILE ============================ */
