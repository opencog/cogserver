/*
 * SexprShell.h
 *
 * Simple s-expression shell
 * Copyright (c) 2008, 2013, 2020 Linas Vepstas <linas@linas.org>
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

#ifndef _OPENCOG_SEXPR_SHELL_H
#define _OPENCOG_SEXPR_SHELL_H

#include <opencog/network/GenericShell.h>

namespace opencog {
/** \addtogroup grp_server
 *  @{
 */

class SexprShell : public GenericShell
{
	public:
		SexprShell(void);
		virtual ~SexprShell();
		virtual GenericEval* get_evaluator(void);
};

/** @}*/
}

#endif // _OPENCOG_SEXPR_SHELL_H
