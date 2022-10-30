/*
 * opencog/cogserver/proxy/ThruProxy.h
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

#ifndef _OPENCOG_THRU_PROXY_H
#define _OPENCOG_THRU_PROXY_H

#include <vector>

#include <opencog/cogserver/proxy/Proxy.h>
#include <opencog/persist/api/StorageNode.h>
#include <opencog/persist/sexpr/Commands.h>

namespace opencog {
/** \addtogroup grp_server
 *  @{
 */

class ThruProxy : public Proxy
{
	protected:
		AtomSpacePtr _as;
		Handle _truth_key;
		std::vector<StorageNodePtr> _targets;

		Commands _decoder;

	public:
		ThruProxy(CogServer&);
		virtual ~ThruProxy();

		virtual void init(void);
		virtual void setup(SexprEval*);
};

/** @}*/
}

#endif // _OPENCOG_THRU_PROXY_H
