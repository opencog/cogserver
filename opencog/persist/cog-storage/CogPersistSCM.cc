/*
 * opencog/persist/cog-storage/CogPersistSCM.cc
 *
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

#include <libguile.h>

#include <opencog/atomspace/AtomSpace.h>
#include <opencog/atomspace/BackingStore.h>
#include <opencog/guile/SchemePrimitive.h>

#include "CogStorage.h"
#include "CogPersistSCM.h"

using namespace opencog;


// =================================================================

CogPersistSCM::CogPersistSCM(AtomSpace *as)
{
    _as = as;
    _backing = nullptr;

    static bool is_init = false;
    if (is_init) return;
    is_init = true;
    scm_with_guile(init_in_guile, this);
}

void* CogPersistSCM::init_in_guile(void* self)
{
    scm_c_define_module("opencog persist-cog", init_in_module, self);
    scm_c_use_module("opencog persist-cog");
    return NULL;
}

void CogPersistSCM::init_in_module(void* data)
{
   CogPersistSCM* self = (CogPersistSCM*) data;
   self->init();
}

void CogPersistSCM::init(void)
{
    define_scheme_primitive("cogserver-open", &CogPersistSCM::do_open, this, "persist-cog");
    define_scheme_primitive("cogserver-close", &CogPersistSCM::do_close, this, "persist-cog");
    define_scheme_primitive("cogserver-stats", &CogPersistSCM::do_stats, this, "persist-cog");
    define_scheme_primitive("cogserver-clear-stats", &CogPersistSCM::do_clear_stats, this, "persist-cog");
}

CogPersistSCM::~CogPersistSCM()
{
    if (_backing) delete _backing;
}

void CogPersistSCM::do_open(const std::string& uri)
{
    if (_backing)
        throw RuntimeException(TRACE_INFO,
             "cogserver-open: Error: Already connected to a database!");

    // Unconditionally use the current atomspace, until the next close.
    AtomSpace *as = SchemeSmob::ss_get_env_as("cogserver-open");
    if (nullptr != as) _as = as;

    if (nullptr == _as)
        throw RuntimeException(TRACE_INFO,
             "cogserver-open: Error: Can't find the atomspace!");

    // Allow only one connection at a time.
    if (_as->isAttachedToBackingStore())
        throw RuntimeException(TRACE_INFO,
             "cogserver-open: Error: Atomspace connected to another storage backend!");
    // Use the CogServer driver.
    CogStorage *store = new CogStorage(uri);
    if (!store)
        throw RuntimeException(TRACE_INFO,
            "cogserver-open: Error: Unable to open the database");

    if (!store->connected())
    {
        delete store;
        throw RuntimeException(TRACE_INFO,
            "cogserver-open: Error: Unable to connect to the database");
    }

    _backing = store;
    _backing->registerWith(_as);
}

void CogPersistSCM::do_close(void)
{
    if (nullptr == _backing)
        throw RuntimeException(TRACE_INFO,
             "cogserver-close: Error: AtomSpace not connected to CogServer!");

    CogStorage *backing = _backing;
    _backing = nullptr;

    // The destructor might run for a while before its done; it will
    // be emptying the pending store queues, which might take a while.
    // So unhook the atomspace first -- this will prevent new writes
    // from accidentally being queued. (It will also drain the queues)
    // Only then actually call the dtor.
    backing->unregisterWith(_as);
    delete backing;
}

void CogPersistSCM::do_stats(void)
{
    if (nullptr == _backing) {
        printf("cogserver-stats: AtomSpace not connected to CogServer!\n");
        return;
    }

    printf("cogserver-stats: Atomspace holds %lu atoms\n", _as->get_size());
    _backing->print_stats();
}

void CogPersistSCM::do_clear_stats(void)
{
    if (nullptr == _backing) {
        printf("cogserver-stats: AtomSpace not connected to CogServer!\n");
        return;
    }

    _backing->clear_stats();
}

void opencog_persist_cog_init(void)
{
    static CogPersistSCM patty(NULL);
}
