/*
 * opencog/cogserver/server/Proxy.h
 *
 * Module to provide a common API for all s-expression proxies.
 * Copyright (c) 2008, 2020 Linas Vepstas <linas@linas.org>
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

#ifndef _OPENCOG_PROXY_H
#define _OPENCOG_PROXY_H

#include <opencog/cogserver/server/Module.h>
#include <opencog/persist/sexpr/SexprEval.h>

namespace opencog {
/** \addtogroup grp_server
 *  @{
 */

class Proxy : public Module
{
	public:
		Proxy(CogServer& cs) : Module(cs) {};
		virtual ~Proxy() {};

		virtual void setup(SexprEval*) = 0;
};

/** @}*/
}

#endif // _OPENCOG_PROXY_H
