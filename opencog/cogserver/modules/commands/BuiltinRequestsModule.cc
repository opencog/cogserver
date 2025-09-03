/*
 * opencog/cogserver/modules/commands/BuiltinRequestsModule.cc
 *
 * Copyright (C) 2008,2010 by OpenCog Foundation
 * Written by Gustavo Gama <gama@vettalabs.com>
 * Written by Joel Pitt <joel@opencog.org>
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

#include <iomanip>
#include <unistd.h>

#include <opencog/util/oc_assert.h>
#include <opencog/cogserver/server/CogServer.h>
#include <opencog/network/ConsoleSocket.h>

#include "BuiltinRequestsModule.h"

using namespace opencog;

//!@{
//! color codes in ANSI
static const char* const COLOR_OFF = "\033[0m";
static const char* const BRIGHT = "\033[1m";
static const char* const RED = "\033[31m";
static const char* const GREEN = "\033[32m";
static const char* const YELLOW = "\033[33m";
static const char* const BLUE = "\033[34m";
static const char* const MAGENTA = "\033[35m";
static const char* const CYAN = "\033[36m";
static const char* const WHITE = "\033[37m";
//!@}

inline void ansi_code(std::string &s,const std::string &code) {
    s.append(code);
}

//!@{
static inline void ansi_off(std::string &s) { ansi_code(s,COLOR_OFF); }
static inline void ansi_bright(std::string &s) { ansi_code(s,BRIGHT); }
static inline void ansi_red(std::string &s) { ansi_code(s,RED); }
static inline void ansi_green(std::string &s) { ansi_code(s,GREEN); }
static inline void ansi_yellow(std::string &s) { ansi_code(s,YELLOW); }
static inline void ansi_blue(std::string &s) { ansi_code(s,BLUE); }
static inline void ansi_magenta(std::string &s) { ansi_code(s,MAGENTA); }
static inline void ansi_cyan(std::string &s) { ansi_code(s,CYAN); }
static inline void ansi_white(std::string &s) { ansi_code(s,WHITE); }
//!@}

DECLARE_MODULE(BuiltinRequestsModule)

BuiltinRequestsModule::BuiltinRequestsModule(CogServer& cs) : Module(cs)
{
    _cogserver.registerRequest(ShutdownRequest::info().id,     &shutdownFactory);
    _cogserver.registerRequest(ConfigModuleRequest::info().id, &configmoduleFactory);
    _cogserver.registerRequest(ListModulesRequest::info().id,  &listmodulesFactory);
    _cogserver.registerRequest(LoadModuleRequest::info().id,   &loadmoduleFactory);
    _cogserver.registerRequest(UnloadModuleRequest::info().id, &unloadmoduleFactory);

    do_help_register();
    do_h_register();

    do_exit_register();
    do_quit_register();
    do_q_register();
    do_ctrld_register();
    do_iaceof_register();
    do_dot_register();

    do_stats_register();

}

BuiltinRequestsModule::~BuiltinRequestsModule()
{
    do_help_unregister();
    do_h_unregister();

    do_exit_unregister();
    do_quit_unregister();
    do_q_unregister();
    do_ctrld_unregister();
    do_iaceof_unregister();
    do_dot_unregister();

    do_stats_unregister();

    _cogserver.unregisterRequest(ShutdownRequest::info().id);
    _cogserver.unregisterRequest(ConfigModuleRequest::info().id);
    _cogserver.unregisterRequest(ListModulesRequest::info().id);
    _cogserver.unregisterRequest(LoadModuleRequest::info().id);
    _cogserver.unregisterRequest(UnloadModuleRequest::info().id);
}

void BuiltinRequestsModule::init()
{
}

// ====================================================================
// Various flavors of closing the connection
std::string BuiltinRequestsModule::do_exit(Request* req, std::list<std::string> args)
{
    ConsoleSocket* con = req->get_console();
    OC_ASSERT(con, "Bad request state");

    con->Exit();

    // After the exit, the pointer to the console will be invalid,
    // so zero it out now, to avoid a bad dereference.  (Note that
    // this call may trigger the ConsoleSocket dtor to run).
    req->set_console(nullptr);
    return "";
}

std::string BuiltinRequestsModule::do_quit(Request *req, std::list<std::string> args)
{
    return do_exit(req, args);
}

std::string BuiltinRequestsModule::do_q(Request *req, std::list<std::string> args)
{
    return do_exit(req, args);
}

std::string BuiltinRequestsModule::do_ctrld(Request *req, std::list<std::string> args)
{
    return do_exit(req, args);
}

std::string BuiltinRequestsModule::do_iaceof(Request *req, std::list<std::string> args)
{
    return do_exit(req, args);
}

std::string BuiltinRequestsModule::do_dot(Request *req, std::list<std::string> args)
{
    return do_exit(req, args);
}

// ====================================================================
// Various flavors of help
std::string BuiltinRequestsModule::do_help(Request *req, std::list<std::string> args)
{
    std::ostringstream oss;

    if (args.empty()) {
        std::list<const char*> commands = _cogserver.requestIds();

        size_t maxl = 0;
        std::list<const char*>::const_iterator it;
        for (it = commands.begin(); it != commands.end(); ++it) {
            size_t len = strlen(*it);
            if (len > maxl) maxl = len;
        }

        oss << "Available commands:" << std::endl;
        for (it = commands.begin(); it != commands.end(); ++it) {
            // Skip hidden commands
            if (_cogserver.requestInfo(*it).hidden) continue;
            std::string cmdname(*it);
            std::string ansi_cmdname;
            ansi_green(ansi_cmdname); ansi_bright(ansi_cmdname);
            ansi_cmdname.append(cmdname);
            ansi_off(ansi_cmdname);
            ansi_green(ansi_cmdname);
            ansi_cmdname.append(":");
            ansi_off(ansi_cmdname);
            size_t cmd_length = strlen(cmdname.c_str());
            size_t ansi_code_length = strlen(ansi_cmdname.c_str()) - cmd_length;
            oss << "  " << std::setw(maxl+ansi_code_length+2) << std::left << ansi_cmdname
                << _cogserver.requestInfo(*it).description << std::endl;
        }
    } else if (args.size() == 1) {
        const RequestClassInfo& cci = _cogserver.requestInfo(args.front());
        if (cci.help != "")
            oss << cci.help << std::endl;
    } else {
        oss << do_helpRequest::info().help << std::endl;
    }

    return oss.str();
}

std::string BuiltinRequestsModule::do_h(Request *req, std::list<std::string> args)
{
    return do_help(req, args);
}

// ====================================================================
// Print general info about server.
std::string BuiltinRequestsModule::do_stats(Request *req, std::list<std::string> args)
{
    return _cogserver.display_stats();
}

// ====================================================================
