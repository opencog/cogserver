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

#include <opencog/cogserver/proxy/WriteThruProxy.h>
#include <opencog/persist/api/StorageNode.h>

namespace opencog {
/** \addtogroup grp_server
 *  @{
 */

class ReadThruProxy : public WriteThruProxy
{
	protected:
		Handle _truth_key;
		std::vector<StorageNodePtr> _targets;

		std::string cog_extract_helper(const std::string&, bool);

		/// Methods that implement the interpreted commands
		// std::string cog_atomspace(const std::string&);
		// std::string cog_atomspace_clear(const std::string&);
		// std::string cog_execute_cache(const std::string&);
		// std::string cog_get_atoms(const std::string&);
		// std::string cog_incoming_by_type(const std::string&);
		// std::string cog_incoming_set(const std::string&);
		// std::string cog_keys_alist(const std::string&);
		// std::string cog_link(const std::string&);
		std::string cog_node(const std::string&);

		// std::string cog_value(const std::string&);
		// std::string cog_ping(const std::string&);
		// std::string cog_version(const std::string&);

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
