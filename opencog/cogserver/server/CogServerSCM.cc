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

#include <thread>
#include <opencog/cogserver/server/CogServer.h>

namespace opencog
{

class CogServerSCM
{
private:
    static void* init_in_guile(void*);
    static void init_in_module(void*);
    void init(void);

    std::string start_server(AtomSpace*, int, int, int, const std::string&,
                             const std::string&, const std::string&);
    std::string stop_server(void);
    Handle set_server_space(AtomSpace*);
    Handle get_server_space(void);

    CogServer* srvr = NULL;
    std::thread * main_loop = NULL;

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

/**
 * Implement a dynamically-loadable cogserver guile module.
 *
 * This is a bare-bones implementation. Things like the server
 * port number, and maybe even the server atomspace, should be
 * configurable from guile, instead of the conf file.
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
    define_scheme_primitive("set-cogserver-atomspace!", &CogServerSCM::set_server_space, this, "cogserver");
    define_scheme_primitive("get-cogserver-atomspace", &CogServerSCM::get_server_space, this, "cogserver");
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
                                       const std::string& scmprompt,
                                       const std::string& cfg)
{
    static std::string rc;

    // Singleton instance. Maybe we should throw, here?
    if (srvr) { rc = "CogServer already running!"; return rc; }

    // Use the config file, if specified.
    if (0 < cfg.size())
    {
        config().load(cfg.c_str(), true);
        telnet_port = config().get_int("SERVER_PORT", telnet_port);
        websocket_port = config().get_int("WEBSOCKET_PORT", websocket_port);
        mcp_port = config().get_int("MCP_PORT", mcp_port);
    }

    // Pass parameters non-locally.
    config().set("ANSI_PROMPT", prompt);
    config().set("ANSI_SCM_PROMPT", scmprompt);

    AtomSpacePtr asp(AtomSpaceCast(as));
    srvr = &cogserver(asp);

    // Load modules specified in config
    srvr->loadModules();

    // Enable the network server and run the server's main loop
    if (0 < telnet_port)
        srvr->enableNetworkServer(telnet_port);
    if (0 < websocket_port)
        srvr->enableWebServer(websocket_port);
    if (0 < mcp_port)
        srvr->enableMCPServer(mcp_port);
    main_loop = new std::thread(&CogServer::serverLoop, srvr);
    rc = "Started CogServer";
    return rc;
}


namespace opencog
{
    // The singleton instance.
    extern CogServer* serverInstance;
};

std::string CogServerSCM::stop_server(void)
{
    static std::string rc;
    // delete singleton instance
    if (NULL == srvr) { rc = "CogServer not running"; return rc;}

    // This is probably not thread-safe...
    srvr->stop();
    main_loop->join();

    srvr->disableNetworkServer();
    delete main_loop;
    delete srvr;
    srvr = NULL;
    serverInstance = nullptr;

    rc = "Stopped CogServer";
    return rc;
}

Handle CogServerSCM::get_server_space()
{
	if (NULL == srvr) return Handle::UNDEFINED;
	return HandleCast(srvr->getAtomSpace());
}

Handle CogServerSCM::set_server_space(AtomSpace* new_as)
{
	if (NULL == srvr) return Handle::UNDEFINED;

	AtomSpacePtr old_as = srvr->getAtomSpace();
	srvr->setAtomSpace(AtomSpaceCast(new_as));
	return HandleCast(old_as);
}
