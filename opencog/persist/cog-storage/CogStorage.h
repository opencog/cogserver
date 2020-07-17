/*
 * FILE:
 * opencog/persist/cog-storage/CogStorage.h
 *
 * FUNCTION:
 * Simple CogServer-backed persistent storage.
 *
 * HISTORY:
 * Copyright (c) 2020 Linas Vepstas <linasvepstas@gmail.com>
 *
 * LICENSE:
 * SPDX-License-Identifier: AGPL-3.0-or-later
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

#ifndef _OPENCOG_COG_STORAGE_H
#define _OPENCOG_COG_STORAGE_H

#include <opencog/atomspace/AtomTable.h>
#include <opencog/atomspace/BackingStore.h>

namespace opencog
{
/** \addtogroup grp_persist
 *  @{
 */

class CogStorage : public BackingStore
{
	private:
		void init(const char *);
		std::string _uri;

		// Socket API
		int _sockfd;
		void do_send(const std::string&);
		std::string do_recv(void);

		// Utilities
		ValuePtr get_value(const std::string&, const std::string&);

	public:
		CogStorage(std::string uri);
		CogStorage(const CogStorage&) = delete; // disable copying
		CogStorage& operator=(const CogStorage&) = delete; // disable assignment
		virtual ~CogStorage();
		bool connected(void); // connection to DB is alive

		void load_atomspace(AtomSpace*, const std::string&);

		void registerWith(AtomSpace*);
		void unregisterWith(AtomSpace*);

		// AtomStorage interface
		Handle getNode(Type, const char *);
		Handle getLink(Type, const HandleSeq&);
		void getIncomingSet(AtomTable&, const Handle&);
		void getIncomingByType(AtomTable&, const Handle&, Type t);
		void storeAtom(const Handle&, bool synchronous = false);
		void removeAtom(const Handle&, bool recursive);
		void loadType(AtomTable&, Type);
		void loadAtomSpace(AtomTable&); // Load entire contents
		void storeAtomSpace(const AtomTable&); // Store entire contents
		void barrier();

		// Debugging and performance monitoring
		void print_stats(void);
		void clear_stats(void); // reset stats counters.
};

/** @}*/
} // namespace opencog

#endif // _OPENCOG_COG_STORAGE_H
