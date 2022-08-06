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
printf("duude new eval ======================================\n");
	_recvd_header = false;
	_sent_header = false;
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
printf("duuude got %x %x >>%s<<\n", expr[0], expr[1], expr.c_str());
	if (0 == expr.size()) return;
	if (0 == expr.compare("\n") or 0 == expr.compare("\r\n"))
	{
printf("duuude received already\n");
		_recvd_header = true;
		return;
	}

	if (0 == expr.compare("json\n")) return;
}

std::string WebEval::poll_result()
{
// printf("enter poll, %d %d\n", _recvd_header, _sent_header);
	if (_recvd_header and not _sent_header)
	{
		_sent_header = true;
		std::string rply =
			"HTTP/1.1 101 Switching Protocols\r\n"
			"Upgrade: websocket\r\n"
			"Connection: Upgrade\r\n"
			"\r\n";
			// Sec-WebSocket-Accept: HSmrc0sMlYUkAGmm5OPpG2HaGWk=
			// Sec-WebSocket-Protocol: chat
		return rply;
	}
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
