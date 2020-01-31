/*
 * opencog/cogserver/server/RequestManager.h
 *
 * Copyright (C) 2002-2007 Novamente LLC
 * Copyright (C) 2008 by OpenCog Foundation
 * Written by Andre Senna <senna@vettalabs.com>
 *            Gustavo Gama <gama@vettalabs.com>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * The Cogserver Request system is deprecated; users are encouraged to
 * explore writing guile (scheme) or python modules instead.
 */

#ifndef _OPENCOG_REQUEST_MANAGER_H
#define _OPENCOG_REQUEST_MANAGER_H

#include <mutex>
#include <vector>

#include <opencog/util/concurrent_queue.h>
#include <opencog/cogserver/server/Factory.h>
#include <opencog/cogserver/server/Request.h>
#include <opencog/cogserver/server/RequestClassInfo.h>

namespace opencog
{
/** \addtogroup grp_server
 *  @{
 */

/**
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
class RequestManager
{
protected:
    std::map<const std::string, AbstractFactory<Request> const*>
        _factories;

    // Container used to store references to requests
    std::map<const std::string, Request*> requests;

    std::mutex processRequestsMutex;
    concurrent_queue<Request*> requestQueue;

public:

    /** RequestManager's constructor. */
    RequestManager(void);

    /** RequestManager's destructor. */
    ~RequestManager(void);

    /**** Request Registry API ****/
    /** Register a new request class/type. Takes the class id and a derived
     *  factory for this particular request type. (note: the caller owns the
     *  factory instance). */
    bool registerRequest(const std::string& id,
                         AbstractFactory<Request> const* factory);

    /** Unregister a request class/type. Takes the class' id. */
    bool unregisterRequest(const std::string& id);

    /** Returns a list with the ids of all the registered request classes. */
    std::list<const char*> requestIds(void) const;

    /** Creates and returns a new instance of a request of class 'id'. */
    Request* createRequest(const std::string& id, CogServer&);

    /** Returns the class metadata from request class 'id'. */
    const RequestClassInfo& requestInfo(const std::string& id) const;

    /**
     * Adds request to the end of the requests queue.
     * Caution: after this push, the request might be executed and
     * deleted in a different thread, and so it must NOT be referenced
     * after the push!
     */
    void pushRequest(Request* request) { requestQueue.push(request); }

    /** Removes and returns the first request from the requests queue. */
    Request* popRequest(void) { return requestQueue.value_pop(); }

    /** Returns the requests queue size. */
    int getRequestQueueSize(void) { return requestQueue.size(); }

    /** Force drain of all outstanding requests */
    void processRequests(void);
}; // class

/** @}*/
}  // namespace

#endif // _OPENCOG_REQUEST_MANAGER_H
