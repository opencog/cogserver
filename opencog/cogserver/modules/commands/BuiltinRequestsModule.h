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
       "Usage: stats\n\n"
       "The current date in UTC is printed, followed by:\n"
       "  up-since: the date when the server was started.\n"
       "  last: the date when the most recent connection was opened.\n"
       "  tot-cnct: grand total number of network connections opened.\n"
       "  cur-open-socks: number of currently open connections.\n"
       "  num-open-fds: number of open file descriptors.\n"
       "  stalls: times that open stalled due to htting max-open-cnt.\n"
       "  cpu: number of CPU seconds used by server.\n"
       "  maxrss: resident set size, in KB. Taken from `getrusage`.\n"
       "\n"
       "The table shows a list of the currently open connections.\n"
       "The table header has the following form:\n"
       "OPEN-DATE THREAD STATE NLINE LAST-ACTIVITY U SHEL QZ E PENDG\n"
       "The columns are:\n"
       "  OPEN-DATE -- when the connection was opened.\n"
       "  THREAD -- the Linux thread-id, as printed by `ps -eLf`\n"
       "  STATE -- several states possible; `iwait` means witing for input.\n"
       "  NLINE -- number of newlines received by the shell.\n"
       "  LAST-ACTIVITY -- the last time anything was received.\n"
       "  U -- use count. The number of active handlers for the socket.\n"
       "  SHEL -- the current shell processor for the socket.\n"
       "  QZ -- size of the unprocessed (pending) request queue.\n"
       "  E -- `T` if the shell evaluator is running, else `F`.\n"
       "  PENDG -- number of bytes of output not yet sent.\n"
       "\n",
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
