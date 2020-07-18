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
#include <opencog/persist/sexpr/Sexpr.h>

#include "CogStorage.h"

using namespace opencog;

void CogStorage::storeAtom(const Handle& h, bool synchronous)
{
	std::string msg = "(cog-set-values! " + Sexpr::encode_atom(h) +
		Sexpr::encode_atom_values(h) + ")\n";
	do_send(msg);

	// Flush the response.
	do_recv();
}

void CogStorage::removeAtom(const Handle& h, bool recursive)
{
	std::string msg;
	if (recursive)
		msg = "(cog-extract-recursive! " + Sexpr::encode_atom(h) + ")\n";
	else
		msg = "(cog-extract! " + Sexpr::encode_atom(h) + ")\n";

	do_send(msg);

	// Flush the response.
	do_recv();
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
	std::string get_keys = "(cog-keys->alist (" + typena + "))\n";
	do_send(get_keys);
	msg = do_recv();
	Sexpr::decode_alist(h, msg);

	return h;
}

Handle CogStorage::getLink(Type t, const HandleSeq& hs)
{
	std::string typena = nameserver().getTypeName(t) + " ";
	for (const Handle& ho: hs)
		typena += Sexpr::encode_atom(ho);

	// Does the cogserver even know about this atom?
	do_send("(cog-link '" + typena + ")\n");
	std::string msg = do_recv();
	if (0 == msg.compare(0, 2, "()"))
		return Handle();

	// Yes, the cogserver knows about this atom
	Handle h = createLink(hs, t);

	// Get all of the keys.
	std::string get_keys = "(cog-keys->alist (" + typena + "))\n";
	do_send(get_keys);
	msg = do_recv();
	Sexpr::decode_alist(h, msg);

	return h;
}

void CogStorage::decode_atom_list(AtomTable& table)
{
	// XXX FIXME .. this WILL fail if the returned list is large.
	// Basically, we don't know quite when all the bytes have been
	// received on the socket... For now, we punt.
	std::string expr = do_recv();

	// Loop and decode atoms.
	size_t l = expr.find('(') + 1; // skip the first paren.
	size_t end = expr.rfind(')');  // trim tailing paren.
	size_t r = end;
	if (l == r) return;
	while (true)
	{
		// get_next_expr() updates the l and r to bracket an expression.
		int pcnt = Sexpr::get_next_expr(expr, l, r, 0);
		if (l == r) break;
		if (0 < pcnt) break;
		table.add(Sexpr::decode_atom(expr, l, r, 0));

		// advance to next.
		l = r+1;
		r = end;
	}
}

void CogStorage::getIncomingSet(AtomTable& table, const Handle& h)
{
	std::string atom = "(cog-incoming-set " + Sexpr::encode_atom(h) + ")\n";
	do_send(atom);
	decode_atom_list(table);
}

void CogStorage::getIncomingByType(AtomTable& table, const Handle& h, Type t)
{
	std::string msg = "(cog-incoming-by-type " + Sexpr::encode_atom(h)
		+ " '" + nameserver().getTypeName(t) + ")\n";
	do_send(msg);
	decode_atom_list(table);
}

void CogStorage::loadAtomSpace(AtomTable &table)
{
	std::string msg = "(cog-get-all-roots)\n";
	do_send(msg);
	decode_atom_list(table);
}

void CogStorage::loadType(AtomTable &table, Type t)
{
	std::string msg = "(cog-get-atoms '" + nameserver().getTypeName(t) + ")\n";
	do_send(msg);
	decode_atom_list(table);
}

void CogStorage::storeAtomSpace(const AtomTable &table)
{
	HandleSet all_roots;
	table.getHandleSetByType(all_roots, ATOM, true);
	for (const Handle& h : all_roots)
		storeAtom(h);
}

void CogStorage::kill_data(void)
{
	do_send("(cog-atomspace-clear)");
	do_recv();
}
