/*
 * opencog/cogserver/server/CogServer.h
 *
 * Copyright (C) 2002-2007 Novamente LLC
 * Copyright (C) 2008 by OpenCog Foundation
 * Written by Andre Senna <senna@vettalabs.com>
 *            Gustavo Gama <gama@vettalabs.com>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef _OPENCOG_COGSERVER_H
#define _OPENCOG_COGSERVER_H

#include <opencog/cogserver/server/Module.h>
#include <opencog/cogserver/server/ModuleManager.h>
#include <opencog/network/NetworkServer.h>
#include <opencog/cogserver/server/BaseServer.h>
#include <opencog/cogserver/server/RequestManager.h>

namespace opencog
{
/** \addtogroup grp_server
 *  @{
 */

/**
 * This class implements a network server. It provides shared
 * network access to a given AtomSpace, together with some basic
 * capabilities to load C++ extension modules.
 *
 * The most useful thing that the cogserver currently provides is
 * shared multi-user network access to a command line, from which
 * the python and scheme read-evaluate-print-loop (REPL) shells
 * can be accessed. This allows users to perform management and
 * development on an atomspace by attaching to it from a remote
 * location.  In particular, the atomspace(s) are not killed when
 * the last user disconnects; the server will stay running in this
 * dettached state, until user reconnect, or until it is shut down.
 *
 * The network server is implemented by devoting one thread to listening
 * for TCP/IP socket connections. Upon connection, a new thread is
 * forked to handle user requests and the run the python/scheme shells.
 * Command-line commands are implemented as "Requests", described below.
 * The AtomSpace is thread-safe, as well as the Request queue, and the
 * python/scheme REPL shells, so there should be no issues with
 * multi-user access.
 */
class CogServer :
    public BaseServer,
    public RequestManager,
    public ModuleManager
{
protected:
    NetworkServer* _consoleServer;
    NetworkServer* _webServer;
    NetworkServer* _mcpServer;
    bool _running;

    /** Protected; singleton instance! Bad things happen when there is
     * more than one. Alas. */
    CogServer(void);
    CogServer(AtomSpacePtr);
friend CogServer& cogserver(void);
friend CogServer& cogserver(AtomSpacePtr);

public:
    /** CogServer's destructor. Disables the network server and
     * unloads all modules. */
    virtual ~CogServer(void);

    /** Server's main loop. Executed while the 'running' flag is set
     *  to true. It processes the request queue.
     */
    virtual void serverLoop(void);

    /** Runs a single server loop step. Quasi-private method, made
     *  public to be used in unit tests and for debug purposes only. */
    virtual void runLoopStep(void);

    /** Terminates the main loop. The loop will be exited
     *  after the current interaction is finished. */
    virtual void stop(void);

    void set_max_open_sockets(int);

    /** Starts the network console server; this provides a command
     *  line server socket on the port specified by the configuration
     *  parameter SERVER_PORT */
    virtual void enableNetworkServer(int port=17001);

	 /** Starts the webscokets server. */
    virtual void enableWebServer(int port=18080);

	 /** Starts the MCP Model Context Protocol server. */
    virtual void enableMCPServer(int port=18888);

    /** Stops the network server and closes all the open server sockets. */
    virtual void disableNetworkServer(void);
    virtual void disableWebServer(void);
    virtual void disableMCPServer(void);

    bool running(void) { return _running; }

    /*** Request API ***/
    Request* createRequest(const std::string& id) {
        return RequestManager::createRequest(id, *this);
    }

    /**** Module API ****/
    bool loadModule(const std::string& filename) {
        return ModuleManager::loadModule(filename, *this);
    }
    void loadModules(void) { ModuleManager::loadModules(*this); }

    /** Return the logger */
    Logger &logger(void) { return opencog::logger(); }


    /** Print human-readable stats about the cogserver */
    std::string display_stats(int nlines = -1);
    std::string display_web_stats(void);
    static std::string stats_legend(void);

}; // class

// Singleton instance of the cogserver
CogServer& cogserver(void);
CogServer& cogserver(AtomSpacePtr);

// Only cython needs this.
inline AtomSpacePtr cython_server_atomspace(void)
{
    return cogserver().getAtomSpace();
}

/** @}*/
}  // namespace

#endif // _OPENCOG_COGSERVER_H
