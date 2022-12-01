/*
 * opencog/cogserver/proxy/ReadThruProxy.h
 *
 * Module for starting up s-expression shells
 * Copyright (c) 2008, 2020, 2022 Linas Vepstas <linas@linas.org>
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

#ifndef _OPENCOG_READ_THRU_PROXY_H
#define _OPENCOG_READ_THRU_PROXY_H

#include <vector>

#include <opencog/cogserver/proxy/Proxy.h>
#include <opencog/cogserver/proxy/ThruCommands.h>

namespace opencog {
/** \addtogroup grp_server
 *  @{
 */

class ReadThru : public ThruCommands
{
	public:
		ReadThru(void);
		~ReadThru();
		void setup(SexprEval*);

		void get_atoms_cb(Type, bool);
		void incoming_by_type_cb(const Handle&, Type);
		void incoming_set_cb(const Handle&);
		void keys_alist_cb(const Handle&);
		void node_cb(const Handle&);
		void link_cb(const Handle&);
		void value_cb(const Handle&, const Handle&);
};

class ReadThruProxy : public Proxy
{
	protected:
		ReadThru _rthru_wrap;

	public:
		ReadThruProxy(CogServer&);
		virtual ~ReadThruProxy();

		static const char *id(void);
		virtual void init(void);
		virtual bool config(const char*);

		virtual void setup(SexprEval*);
};

/** @}*/
}

#endif // _OPENCOG_READ_THRU_PROXY_H
