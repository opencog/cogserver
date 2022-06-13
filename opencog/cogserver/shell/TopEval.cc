/**
 * TopEval.cc
 *
 * Report cogserver stats.
 *
 * Copyright (c) 2008, 2014, 2015, 2020, 2021, 2022 Linas Vepstas
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

#include <unistd.h>
#include "TopEval.h"

using namespace opencog;

TopEval::TopEval()
	: GenericEval()
{
	_started = false;
	_done = false;
	_refresh = 3;
}

TopEval::~TopEval()
{
}

/* ============================================================== */
/**
 * Evaluate an basic Javascript/JSON commands.
 */
void TopEval::eval_expr(const std::string &expr)
{
printf("duuude eval >>%s<<\n", expr.c_str());
}

std::string TopEval::poll_result()
{
	if (_done) return "";

	if (_started) sleep(_refresh);
	else _started = true;

	std::string ret = "\u001B[2Jx-- yo ";
static int cnt = 0;
cnt++;
ret += std::to_string(cnt);
	ret += "\n";
	return ret;
}

void TopEval::begin_eval()
{
printf("duuude begin eval\n");
}

/* ============================================================== */

/**
 * interrupt() - convert user's control-C at keyboard into exception.
 */
void TopEval::interrupt(void)
{
	_done = true;
	_caught_error = true;
printf("duuude git interrupts\n");
}

TopEval* TopEval::get_evaluator()
{
	static TopEval* evaluator = new TopEval();

printf("duuuude return evaluator\n");
	return evaluator;
}

/* ===================== END OF FILE ======================== */
