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

#include <chrono>
#include "TopEval.h"

using namespace opencog;
using namespace std::chrono_literals;

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
	// Right now, we don't have any built-in commands ...
	// if we did, they'd be handled here.
}

std::string TopEval::poll_result()
{
	if (_done) return "";

	// Sleep for three seconds, every time except the first time.
	// Issue a newline the first time to help clear the screen.
	std::string ret;
	if (_started)
	{
		std::unique_lock<std::mutex> lck(_sleep_mtx);
		_sleeper.wait_for(lck, _refresh * 1s);
	}
	else
	{
		_started = true;
		ret = "\n";
	}

	ret += "\u001B[2Jx-- yo ";
static thread_local int cnt = 0;
cnt++;
ret += std::to_string(cnt);
	ret += "\n";
	return ret;
}

void TopEval::begin_eval()
{
	_done = false;
}

// When the user types something while top is running, this gets called.
// This will halt the polling loop, and allow the user input to be
// processed.
void TopEval::cmd()
{
	_done = true;
	_sleeper.notify_all();
}

/* ============================================================== */

/**
 * interrupt() - convert user's control-C at keyboard into exception.
 */
void TopEval::interrupt(void)
{
	_done = true;
	_caught_error = true;
	_sleeper.notify_all();
}

// One evaluator per thread.  This allows multiple users to each
// have thier own evaluator.
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
