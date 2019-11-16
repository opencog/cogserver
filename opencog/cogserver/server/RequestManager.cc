/*
 * opencog/cogserver/server/RequestManager.cc
 *
 * Copyright (C) 2002-2007 Novamente LLC
 * Copyright (C) 2008 by OpenCog Foundation
 * Written by Andre Senna <senna@vettalabs.com>
 *            Gustavo Gama <gama@vettalabs.com>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <time.h>
#include <unistd.h>
#include <sys/time.h>

#include <opencog/util/Logger.h>
#include <opencog/util/exceptions.h>
#include <opencog/util/misc.h>
#include <opencog/util/platform.h>

#include <opencog/cogserver/server/Request.h>

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
    return Registry<Request>::register_(name, factory);
}

bool RequestManager::unregisterRequest(const std::string& name)
{
    return Registry<Request>::unregister(name);
}

Request* RequestManager::createRequest(const std::string& name,
                                       CogServer& cs)
{
    return Registry<Request>::create(cs, name);
}

const RequestClassInfo& RequestManager::requestInfo(const std::string& name) const
{
    return static_cast<const RequestClassInfo&>(Registry<Request>::classinfo(name));
}

std::list<const char*> RequestManager::requestIds() const
{
    return Registry<Request>::all();
}

// =============================================================
