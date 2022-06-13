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
	init();
}

TopEval::~TopEval()
{
}

void TopEval::init()
{
	_started = false;
	_done = false;
	_refresh = 3;
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
static thread_local int cnt = 0;
cnt++;
ret += std::to_string(cnt);
	ret += "\n";
	return ret;
}

void TopEval::begin_eval()
{
printf("duuude begin eval\n");
	_done = false;
}

// When the user types something while top is rinning, this gets called.
// This will halt the polling loop, and allow the user input to be
// processed.
void TopEval::cmd()
{
	_done = true;
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
	static thread_local TopEval* evaluator = new TopEval();

	// The eval_dtor runs when this thread is destroyed.
	class eval_dtor {
		public:
		~eval_dtor() { delete evaluator; }
	};
	static thread_local eval_dtor killer;

	return evaluator;
}

/* ===================== END OF FILE ======================== */
