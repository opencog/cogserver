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

#include <opencog/cogserver/server/CogServer.h>
#include "TopEval.h"

using namespace opencog;
using namespace std::chrono_literals;

TopEval::TopEval()
	: GenericEval()
{
	_started = false;
	_done = false;
	_refresh = 3.0;
	_nlines = 24;     // For a standard 24x80 terminal
	_msg.reserve(80);
}

TopEval::~TopEval()
{
}

/* ============================================================== */
/**
 * Evaluate commands appropriate for top.
 */
void TopEval::eval_expr(const std::string &expr)
{
	if (0 == expr.size()) return;
	if (0 == expr.compare("\n")) return;

	// On startup, the initial message sent is "foo".
	// This is in TopShellModule();
	if (0 == expr.compare("foo\n")) return;

	if ('h' == expr[0])
	{
		_msg = "Available commands: display kill";
		return;
	}

	// Kill a thread
	if ('k' == expr[0])
	{
		size_t pos = expr.find_first_of(" ");
		if (std::string::npos == pos)
		{
			_msg = "Usage: kill <thread-id>";
			return;
		}

		size_t tid = std::atoi(expr.substr(pos).c_str());
		bool rc = ServerSocket::kill(tid);
		if (rc)
			_msg = "Killed thread " + std::to_string(tid);
		else
			_msg = "No such thread " + std::to_string(tid);
		return;
	}

	// Change number of display lines
	if ('d' == expr[0])
	{
		size_t pos = expr.find_first_of(" ");
		if (std::string::npos == pos)
		{
			_msg = "Usage: display <num-lines-to-display>";
			return;
		}

		_nlines = std::atoi(expr.substr(pos).c_str());
		return;
	}

	// Wasn't one of the above.
	_msg = "Unknown top command >>" + expr;
}

std::string TopEval::poll_result()
{
	if (_done) return "";

	// Sleep for three seconds, every time except the first time.
	// Issue a newline the first time to help clear the screen.
	std::string ret;
	ret.reserve(4000);
	if (_started)
	{
		std::unique_lock<std::mutex> lck(_sleep_mtx);
		_sleeper.wait_for(lck, ((int) (1000 * _refresh)) * 1ms);
	}
	else
	{
		_started = true;
		ret = "\n";
	}

	// Send the telnet clear-screen command.
	ret += "\u001B[2J";
	ret += cogserver().display_stats(_nlines);

	if (0 < _msg.size())
	{
		ret += _msg;
		_msg = "";
	}
	return ret;
}

void TopEval::begin_eval()
{
	_done = false;
}

/* ============================================================== */

void TopEval::set_interval(double secs)
{
	_refresh = secs;
}

/* ============================================================== */

// When the user types something while top is running, this gets called.
// This will halt the polling loop, and allow the user input to be
// processed.
void TopEval::cmd()
{
	_done = true;
	_started = false;
	_sleeper.notify_all();
}

/**
 * interrupt() - convert user's control-C at keyboard into exception.
 */
void TopEval::interrupt(void)
{
	_done = true;
	_started = false;
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
