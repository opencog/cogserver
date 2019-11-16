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

#include <memory>
#include <mutex>
#include <vector>
#include <thread>

#include <opencog/util/concurrent_queue.h>
#include <opencog/cogserver/server/Module.h>
#include <opencog/cogserver/server/ModuleManager.h>
#include <opencog/network/NetworkServer.h>
#include <opencog/cogserver/server/BaseServer.h>
#include <opencog/cogserver/server/Request.h>
#include <opencog/cogserver/server/Registry.h>

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
 *
 * -----------------------------------------------------------------
 * Implementation details:
 * 
 * Request management uses the Registry base template, specialized
 * with the Request base class. The functionalities provided are:
 *   1. register, unregister and list request classes;
 *   2. create requests instances;
 *   3. push/pop from requests queue.
 * The lifecycle of each Request is controlled by the server itself
 * (that why no "destroyRequest" is provided). The server destroys the
 * Request instance right after its execution. All requests are assumed
 * to run to completion in a small amount of time, so that they don't
 * block later requests.
 *
 * This constraint does not apply to the Python and Scheme shells:
 * Work there can run for unlimited amounts of time. All that happens
 * there is that the shell client is blocked, until the job finshes
 * or until they kill it.  The GenericShell suppoprts out-of-band
 * ctrl-C, so the user can always ctrl-C to kill an out-of-control
 * Scheme or Python job.
 */
class CogServer :
    public BaseServer,
    public Registry<Request>,
    public ModuleManager
{
protected:

    // Container used to store references to requests
    std::map<const std::string, Request*> requests;

    std::mutex processRequestsMutex;
    concurrent_queue<Request*> requestQueue;

    NetworkServer* _networkServer;
    bool _running;

public:

    /** CogServer's constructor. */
    CogServer(AtomSpace* = nullptr);

    /** Factory method. Override's the base class factory method
     * and returns an instance of CogServer instead. */
    static BaseServer* createInstance(AtomSpace* = nullptr);

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

    /** Starts the network server and adds the default command line
     *  server socket on the port specified by the configuration
     *  parameter SERVER_PORT */
    virtual void enableNetworkServer(int port=17001);

    /** Stops the network server and closes all the open server sockets. */
    virtual void disableNetworkServer(void);

    /**** Request Registry API ****/
    /** Register a new request class/type. Takes the class id and a derived
     *  factory for this particular request type. (note: the caller owns the
     *  factory instance). */
    virtual bool registerRequest(const std::string& id,
                                 AbstractFactory<Request> const* factory);

    /** Unregister a request class/type. Takes the class' id. */
    virtual bool unregisterRequest(const std::string& id);

    /** Returns a list with the ids of all the registered request classes. */
    virtual std::list<const char*> requestIds(void) const;

    /** Creates and returns a new instance of a request of class 'id'. */
    virtual Request* createRequest(const std::string& id);

    /** Returns the class metadata from request class 'id'. */
    virtual const RequestClassInfo& requestInfo(const std::string& id) const;

    /**
     * Adds request to the end of the requests queue.
     * Caution: after this push, the request might be executed and
     * deleted in a different thread, and so it must NOT be referenced
     * after the push!
     */
    void pushRequest(Request* request) { requestQueue.push(request); }

    /** Removes and returns the first request from the requests queue. */
    Request* popRequest(void) { return requestQueue.pop(); }

    /** Returns the requests queue size. */
    int getRequestQueueSize(void) { return requestQueue.size(); }

    /** Force drain of all outstanding requests */
    void processRequests(void);

    /**** Module API ****/
    bool loadModule(const std::string& filename) {
        return ModuleManager::loadModule(filename, *this);
    }
    void loadModules(void) { ModuleManager::loadModules(*this); }

    /** Return the logger */
    Logger &logger(void);

}; // class

// Handy dandy utility
inline CogServer& cogserver(AtomSpace* as = nullptr)
{
    return dynamic_cast<CogServer&>(server(CogServer::createInstance, as));
}

/** @}*/
}  // namespace

#endif // _OPENCOG_COGSERVER_H
