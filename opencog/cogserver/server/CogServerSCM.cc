/*
 * CogServerSCM.cc
 *
 * Copyright (C) 2015 OpenCog Foundation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License v3 as
 * published by the Free Software Foundation and including the
 * exceptions
 * at http://opencog.org/wiki/Licenses
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public
 * License
 * along with this program; if not, write to:
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

// The header file bits.

#ifndef _OPENCOG_COGSERVER_SCM_H
#define _OPENCOG_COGSERVER_SCM_H

#include <opencog/cogserver/atoms/CogServerNode.h>

namespace opencog
{

class CogServerSCM
{
private:
    static void* init_in_guile(void*);
    static void init_in_module(void*);
    void init(void);

    std::string start_server(AtomSpace*, int, int, int, const std::string&,
                             const std::string&);
    std::string stop_server(void);

    CogServerNode* srvr = nullptr;

public:
    CogServerSCM();
};


extern "C" {
void opencog_cogserver_init(void);
};

}
#endif // _OPENCOG_COGSERVER_SCM_H

// --------------------------------------------------------------

#include <opencog/util/Config.h>
#include <opencog/guile/SchemePrimitive.h>
#include <opencog/atoms/atom_types/atom_names.h>
#include <opencog/atoms/value/FloatValue.h>
#include <opencog/atoms/value/StringValue.h>
#include <opencog/atoms/value/VoidValue.h>

/**
 * Implement a dynamically-loadable cogserver guile module.
 *
 * An interesting idea for the future is to convert this to
 * an instance of ObjectNode, so that the configurable parameters
 * could be set with Atomese-- i.e. with SetValue or cog-set-value!
 * by sending messages such as (Predicate "*-web-port-number-"")
 * and similar.
 */

using namespace opencog;

CogServerSCM::CogServerSCM()
{
    static bool is_init = false;
    if (is_init) return;
    is_init = true;
    scm_with_guile(init_in_guile, this);
}

void* CogServerSCM::init_in_guile(void* self)
{
    scm_c_define_module("opencog cogserver", init_in_module, self);
    scm_c_use_module("opencog cogserver");
    return NULL;
}

void CogServerSCM::init_in_module(void* data)
{
    CogServerSCM* self = (CogServerSCM*) data;
    self->init();
}

/**
 * The main init function for the CogServerSCM object.
 */
void CogServerSCM::init()
{
    define_scheme_primitive("c-start-cogserver", &CogServerSCM::start_server, this, "cogserver");
    define_scheme_primitive("c-stop-cogserver", &CogServerSCM::stop_server, this, "cogserver");
}

extern "C" {
void opencog_cogserver_init(void)
{
    static CogServerSCM cogserver_bindings;
}
};

// --------------------------------------------------------------

std::string CogServerSCM::start_server(AtomSpace* as,
                                       int telnet_port,
                                       int websocket_port,
                                       int mcp_port,
                                       const std::string& prompt,
                                       const std::string& scmprompt)
{
    static std::string rc;

    // Only one server at a time.
    if (srvr) { rc = "CogServer already running!"; return rc; }

    srvr = new CogServerNode("cogserver");

    // Set non-default port values
    if (telnet_port != 17001)
        srvr->setValue(as->add_atom(Predicate("*-telnet-port-*")),
                       createFloatValue((double)telnet_port));
    if (websocket_port != 18080)
        srvr->setValue(as->add_atom(Predicate("*-web-port-*")),
                       createFloatValue((double)websocket_port));
    if (mcp_port != 18888)
        srvr->setValue(as->add_atom(Predicate("*-mcp-port-*")),
                       createFloatValue((double)mcp_port));

    // Set custom prompts
    if (!prompt.empty())
        srvr->setValue(as->add_atom(Predicate("*-ansi-prompt-*")),
                       createStringValue(prompt));
    if (!scmprompt.empty())
        srvr->setValue(as->add_atom(Predicate("*-ansi-scm-prompt-*")),
                       createStringValue(scmprompt));

    // Start all servers
    srvr->setValue(as->add_atom(Predicate("*-start-*")),
                   createVoidValue());
    rc = "Started CogServer";
    return rc;
}


std::string CogServerSCM::stop_server(void)
{
    static std::string rc;
    if (nullptr == srvr) { rc = "CogServer not running"; return rc;}

    AtomSpacePtr asp = srvr->getAS();
    srvr->setValue(asp->add_atom(Predicate("*-stop-*")),
                   createVoidValue());
    delete srvr;
    srvr = nullptr;

    rc = "Stopped CogServer";
    return rc;
}
