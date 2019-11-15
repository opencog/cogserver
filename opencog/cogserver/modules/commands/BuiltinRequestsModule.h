/*
 * opencog/cogserver/modules/commands/BuiltinRequestsModule.h
 *
 * Copyright (C) 2008 by OpenCog Foundation
 * Written by Gustavo Gama <gama@vettalabs.com>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef _OPENCOG_BUILTIN_REQUESTS_MODULE_H
#define _OPENCOG_BUILTIN_REQUESTS_MODULE_H

#include <opencog/cogserver/server/Factory.h>
#include <opencog/cogserver/server/Module.h>
#include <opencog/cogserver/modules/commands/ListRequest.h>
#include <opencog/cogserver/modules/commands/LoadModuleRequest.h>
#include <opencog/cogserver/modules/commands/ShutdownRequest.h>
#include <opencog/cogserver/modules/commands/UnloadModuleRequest.h>
#include <opencog/cogserver/modules/commands/ListModulesRequest.h>

#include <opencog/util/Logger.h>
#include <opencog/cogserver/server/CogServer.h>

namespace opencog
{
/** \addtogroup grp_server
 *  @{
 */

class BuiltinRequestsModule : public Module
{

private:

    Factory<ListRequest, Request>         listFactory;
    Factory<ShutdownRequest, Request>     shutdownFactory;
    Factory<LoadModuleRequest, Request>   loadmoduleFactory;
    Factory<UnloadModuleRequest, Request> unloadmoduleFactory;
    Factory<ListModulesRequest, Request>  listmodulesFactory;

DECLARE_CMD_REQUEST(BuiltinRequestsModule, "exit", do_exit,
       "Close the shell connection",
       "Usage: exit\n\n"
       "Close the shell TCP/IP connection.\n",
       false, true)

DECLARE_CMD_REQUEST(BuiltinRequestsModule, "quit", do_quit,
       "Close the shell connection",
       "Usage: quit\n\n"
       "Close the shell TCP/IP connection.\n",
       false, false)

DECLARE_CMD_REQUEST(BuiltinRequestsModule, "q", do_q,
       "Close the shell connection",
       "Usage: q\n\n"
       "Close the shell TCP/IP connection.\n",
       false, true)

DECLARE_CMD_REQUEST(BuiltinRequestsModule, "", do_ctrld,
       "Close the shell connection",
       "Usage: ^D\n\n"
       "Close the shell TCP/IP connection.\n",
       false, true)

DECLARE_CMD_REQUEST(BuiltinRequestsModule, ".", do_dot,
       "Close the shell connection",
       "Usage: .\n\n"
       "Close the shell TCP/IP connection.\n",
       false, true)

DECLARE_CMD_REQUEST(BuiltinRequestsModule, "help", do_help,
       "List the available commands or print the help for a specific command",
       "Usage: help [<command>]\n\n"
       "If no command is specified, then print a menu of commands.\n"
       "Otherwise, print verbose help for the indicated command.\n",
       false, false)

DECLARE_CMD_REQUEST(BuiltinRequestsModule, "h", do_h,
       "List the available commands or print the help for a specific command",
       "Usage: h [<command>]\n\n"
       "If no command is specified, then print a menu of commands.\n"
       "Otherwise, print verbose help for the indicated command.\n",
       false, true)

DECLARE_CMD_REQUEST(BuiltinRequestsModule, "stats", do_stats,
       "Print some diagnostic statistics about the server.",
       "Usage: stats\n\n",
       false, false)

public:
    static const char* id();
    BuiltinRequestsModule(CogServer&);
    virtual ~BuiltinRequestsModule();
    virtual void init();

}; // class

/** @}*/
}  // namespace

#endif // _OPENCOG_BUILTIN_REQUESTS_MODULE_H
