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

#include "TopEval.h"
#include "TopShell.h"

using namespace opencog;

TopShell::TopShell(void)
{
	normal_prompt = "top> ";
	abort_prompt = "";
	pending_prompt = "";

	show_prompt = true;
	_name = "top ";

	_refresh = 3.0;
	_top_eval = nullptr;
}

TopShell::~TopShell()
{
printf("duuude top shel dtor running\n");
}

void TopShell::set_interval(double refresh)
{
	_refresh = refresh;
}

// When user types in ctrl-C, just exit the shell.
void TopShell::user_interrupt()
{
printf("duuude top shell caught ctrl -C\n");
	self_destruct = true;
	GenericShell::user_interrupt();
printf("duuude top shell done with ctrl -C\n");
}

void TopShell::line_discipline(const std::string& expr)
{
printf("duuude top shell entr line dics\n");
	_top_eval->cmd();
	GenericShell::line_discipline(expr);
printf("duuude top shell exit line dics\n");
}

GenericEval* TopShell::get_evaluator(void)
{
	_top_eval = TopEval::get_evaluator();
	_top_eval->set_interval(_refresh);
	return _top_eval;
}

/* ===================== END OF FILE ============================ */
