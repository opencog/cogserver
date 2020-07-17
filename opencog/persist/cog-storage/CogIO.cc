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
#include <opencog/atomspace/AtomSpace.h>

#include "CogStorage.h"

using namespace opencog;

void CogStorage::storeAtom(const Handle& h, bool synchronous)
{
}

void CogStorage::removeAtom(const Handle& atom, bool recursive)
{
}

Handle CogStorage::getNode(Type t, const char * str)
{
	return Handle();
}

Handle CogStorage::getLink(Type t, const HandleSeq& hs)
{
	return Handle();
}

void CogStorage::loadType(AtomTable &table, Type atom_type)
{
}

void CogStorage::getIncomingSet(AtomTable& table, const Handle& h)
{
}

void CogStorage::getIncomingByType(AtomTable& table, const Handle& h, Type t)
{
}

void CogStorage::loadAtomSpace(AtomTable &table)
{
}

void CogStorage::storeAtomSpace(const AtomTable &table)
{
}

void CogStorage::load_atomspace(AtomSpace* as,
                                const std::string& spacename)
{
}
