/*
 * WebEval.h
 *
 * A simple WebSockets handler.
 * Copyright (c) 2008, 2013, 2014, 2020, 2021, 2022 Linas Vepstas <linas@linas.org>
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

#ifndef _OPENCOG_WEB_EVAL_H
#define _OPENCOG_WEB_EVAL_H

#include <string>
#include <opencog/atomspace/AtomSpace.h>
#include <opencog/eval/GenericEval.h>

/**
 * The WebEval class implements a very simple API for handling WebSocket
 * handshakes.
 */

namespace opencog {
/** \addtogroup grp_server
 *  @{
 */

class WebEval : public GenericEval
{
	private:
		WebEval();
		bool _recvd_header;
		bool _sent_header;

	public:
		virtual ~WebEval();

		virtual void begin_eval(void);
		virtual void eval_expr(const std::string&);
		virtual std::string poll_result(void);

		virtual void interrupt(void);

		static WebEval* get_evaluator(AtomSpace*);
};

/** @}*/
}

#endif // _OPENCOG_WEB_EVAL_H
