/**
 * WebEval.cc
 *
 * Simple WebSockets handler.
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

#include <opencog/cogserver/server/CogServer.h>
#include "WebEval.h"

using namespace opencog;

WebEval::WebEval()
	: GenericEval()
{
}

WebEval::~WebEval()
{
}

/* ============================================================== */
/**
 * Evaluate HTTP handshake.
 */
void WebEval::eval_expr(const std::string &expr)
{
printf("duuude got >>%s<<\n", expr.c_str());
	if (0 == expr.size()) return;
	if (0 == expr.compare("\n")) return;

	// On startup, the initial message sent is "foo".
	// This is in WebShellModule();
	if (0 == expr.compare("foo\n")) return;

	// Right now, we don't have any built-in commands ...
	// if we did, they'd be handled here.
	// _msg = "Unknown web command >>" + expr;
}

std::string WebEval::poll_result()
{
	return "";
}

void WebEval::begin_eval()
{
printf("duuude begin eval\n");
}

/**
 * interrupt() - convert user's control-C at keyboard into exception.
 */
void WebEval::interrupt(void)
{
	_caught_error = true;
}

// One evaluator per thread.  This allows multiple users to each
// have thier own evaluator.
WebEval* WebEval::get_evaluator(AtomSpace* as)
{
	static thread_local WebEval* evaluator = new WebEval();

	// The eval_dtor runs when this thread is destroyed.
	class eval_dtor {
		public:
		~eval_dtor() { delete evaluator; }
	};
	static thread_local eval_dtor killer;

	return evaluator;
}

/* ===================== END OF FILE ======================== */
