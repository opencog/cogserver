/*
 * TopShell.cc
 *
 * Simple server statistics shell
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

#include <opencog/cogserver/server/CogServer.h>

#include "TopShell.h"

using namespace opencog;

TopShell::TopShell(void)
{
	normal_prompt = "top> ";
	abort_prompt = "top> ";
	pending_prompt = "... ";

	show_prompt = true;
	_name = "top";

	_refresh = 3;
}

TopShell::~TopShell()
{
}

void TopShell::set_interval(int refresh)
{
	_refresh = refresh;
}

GenericEval* TopShell::get_evaluator(void)
{
	// return TopEval::get_evaluator(&cogserver().getAtomSpace());
	return nullptr;
}

/* ===================== END OF FILE ============================ */
