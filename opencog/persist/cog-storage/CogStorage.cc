/*
 * FILE:
 * opencog/persist/cog-storage/CogStorage.cc
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

#include "CogStorage.h"

using namespace opencog;

/// This is a cheap, simple, super-low-brow atomspace server
/// built on the cogserver. Its not special. It's simple.
/// It is meant to be replaced by something better.

/* ================================================================ */
// Constructors

void CogStorage::init(const char * uri)
{
#define URIX_LEN (sizeof("cog://") - 1)  // Should be 6
	if (strncmp(uri, "cog://", URIX_LEN))
		throw IOException(TRACE_INFO, "Unknown URI '%s'\n", uri);

	// We expect the URI to be for the form
	//    cog://ipv4-addr/atomspace-name
	//    cog://ipv4-addr:port/atomspace-name
}

CogStorage::CogStorage(std::string uri)
{
	init(uri.c_str());
}

CogStorage::~CogStorage()
{
}

bool CogStorage::connected(void)
{
	return true;
}

/* ================================================================== */
/// Drain the pending store queue. This is a fencing operation; the
/// goal is to make sure that all writes that occurred before the
/// barrier really are performed before before all the writes after
/// the barrier.
///
void CogStorage::barrier()
{
}

/* ================================================================ */

void CogStorage::registerWith(AtomSpace* as)
{
	BackingStore::registerWith(as);
}

void CogStorage::unregisterWith(AtomSpace* as)
{
	BackingStore::unregisterWith(as);
}

/* ================================================================ */

void CogStorage::clear_stats(void)
{
}

void CogStorage::print_stats(void)
{
}

/* ============================= END OF FILE ================= */
