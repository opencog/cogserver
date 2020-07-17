/*
 * CogIO.cc
 * Save/restore of individual atoms.
 *
 * Copyright (c) 2020 Linas Vepstas <linas@linas.org>
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

#include <opencog/atoms/base/Atom.h>
#include <opencog/atoms/base/Node.h>
#include <opencog/atoms/base/Link.h>
#include <opencog/atoms/value/Value.h>
#include <opencog/atomspace/AtomSpace.h>

#include "CogStorage.h"

using namespace opencog;

void CogStorage::storeAtom(const Handle& h, bool synchronous)
{
	throw RuntimeException(TRACE_INFO, "Not implemented!");
}

void CogStorage::removeAtom(const Handle& atom, bool recursive)
{
	throw RuntimeException(TRACE_INFO, "Not implemented!");
}

/// Get the Value on the Atom at Key.
ValuePtr CogStorage::get_value(const std::string& atom,
                               const std::string& key)
{
printf("asking for key >>%s<<\n", key.c_str());
	// This should not fail.
	do_send("(cog-value (" + atom + ")" + key + ")\n");
	std::string msg = do_recv();

printf("got value %s\n", msg.c_str());
// Use DHTAtomStorage::decodeStrValue() here ...
	return nullptr;
}

Handle CogStorage::getNode(Type t, const char * str)
{
	std::string typena = nameserver().getTypeName(t) + " \"" + str + "\"";

	// Does the cogserver even know about this atom?
	do_send("(cog-node '" + typena + ")\n");
	std::string msg = do_recv();
	if (0 == msg.compare(0, 2, "()"))
		return Handle();

	// Yes, the cogserver knows about this atom
	Handle h = createNode(t, str);

	// Get all of the keys.
	std::string get_keys = "(cog-keys (" + typena + "))\n";
	do_send(get_keys);
	msg = do_recv();
	if (0 == msg.compare(0, 2, "()"))
		return h;

	// Loop over all the keys.
	// This is quasi-fragile, depending on what the server returned.
	// But it will do for now.
	size_t first = 1;
	while (std::string::npos != first)
	{
		size_t last = msg.find(')', first);
		std::string key = msg.substr(first, last);
		ValuePtr v = get_value(typena, key);
		first = msg.find('(', last);
	}

	return h;
}

Handle CogStorage::getLink(Type t, const HandleSeq& hs)
{
	throw RuntimeException(TRACE_INFO, "Not implemented!");
	return Handle();
}

void CogStorage::loadType(AtomTable &table, Type atom_type)
{
	throw RuntimeException(TRACE_INFO, "Not implemented!");
}

void CogStorage::getIncomingSet(AtomTable& table, const Handle& h)
{
	throw RuntimeException(TRACE_INFO, "Not implemented!");
}

void CogStorage::getIncomingByType(AtomTable& table, const Handle& h, Type t)
{
	throw RuntimeException(TRACE_INFO, "Not implemented!");
}

void CogStorage::loadAtomSpace(AtomTable &table)
{
	throw RuntimeException(TRACE_INFO, "Not implemented!");
}

void CogStorage::storeAtomSpace(const AtomTable &table)
{
	throw RuntimeException(TRACE_INFO, "Not implemented!");
}

void CogStorage::load_atomspace(AtomSpace* as,
                                const std::string& spacename)
{
	throw RuntimeException(TRACE_INFO, "Not implemented!");
}
