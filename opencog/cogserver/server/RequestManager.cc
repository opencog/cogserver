/*
 * opencog/cogserver/server/RequestManager.cc
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

#include <time.h>
#include <unistd.h>
#include <sys/time.h>

#include <opencog/util/Logger.h>
#include <opencog/util/exceptions.h>
#include <opencog/util/misc.h>
#include <opencog/util/platform.h>

#include <opencog/cogserver/server/CogServer.h>
#include "RequestManager.h"

using namespace opencog;

RequestManager::~RequestManager()
{
}

RequestManager::RequestManager(void)
{
}

// =============================================================

void RequestManager::processRequests(void)
{
    std::lock_guard<std::mutex> lock(processRequestsMutex);
    while (0 < getRequestQueueSize()) {
        Request* request = popRequest();
        request->execute();
        delete request;
    }
}

// =============================================================
// Request registration

bool RequestManager::registerRequest(const std::string& name,
                                     AbstractFactory<Request> const* factory)
{
    return _factories.insert({name, factory}).second;
}

bool RequestManager::unregisterRequest(const std::string& name)
{
    return _factories.erase(name) == 1;
}

Request* RequestManager::createRequest(const std::string& name,
                                       CogServer& cs)
{
    const auto it = _factories.find(name);
    if (it == _factories.end()) {
        // Probably a user typo at the server prompt.
        logger().debug("Cannot create unknown request \"%s\"", name.c_str());
        return nullptr;
    }
    return it->second->create(cogserver());
}

const RequestClassInfo& RequestManager::requestInfo(const std::string& name) const
{
    static RequestClassInfo emptyClassInfo;
    const auto it = _factories.find(name);
    if (it == _factories.end()) {
        // Probably a user typo at the server prompt.
        logger().debug("No info about unknown request \"%s\"", name.c_str());
        return emptyClassInfo;
    }
    return static_cast<const RequestClassInfo&>(it->second->info());
}

std::list<const char*> RequestManager::requestIds() const
{
    std::list<const char*> l;
    for (const auto& fact : _factories)
        l.push_back(fact.first.c_str());
    return l;
}

// =============================================================
