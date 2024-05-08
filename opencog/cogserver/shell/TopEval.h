/*
 * TopEval.h
 *
 * A simple server-stats evaluator
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

#ifndef _OPENCOG_TOP_EVAL_H
#define _OPENCOG_TOP_EVAL_H

#include <condition_variable>
#include <mutex>
#include <string>
#include <opencog/eval/GenericEval.h>

/**
 * The TopEval class implements a very simple API for reporting server
 * stats.
 */

namespace opencog {
/** \addtogroup grp_server
 *  @{
 */

class TopEval : public GenericEval
{
	private:
		std::mutex _sleep_mtx;
		std::condition_variable _sleeper;
		double _refresh;
		int _nlines;
		bool _started;
		bool _done;
		std::string _msg;

		TopEval();

	public:
		virtual ~TopEval();

		virtual void begin_eval(void);
		virtual void eval_expr(const std::string&);
		virtual std::string poll_result(void);

		virtual void interrupt(void);

		void cmd();
		void set_interval(double);

		static TopEval* get_evaluator();
};

/** @}*/
}

#endif // _OPENCOG_TOP_EVAL_H
