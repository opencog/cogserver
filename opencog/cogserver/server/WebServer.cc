/*
 * opencog/cogserver/server/WebServer.cc
 *
 * Copyright (C) 2022 Linas Vepstas
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <string>

#include <opencog/util/Logger.h>
#include <opencog/util/misc.h>

#include <opencog/cogserver/server/CogServer.h>
#include <opencog/cogserver/server/WebServer.h>

using namespace opencog;

WebServer::WebServer(void)
{
printf("duuude web ctor\n");
}

WebServer::~WebServer()
{
printf("duuude web dtor\n");
}

void WebServer::OnConnection(void)
{
printf("duude connect\n");
}

void WebServer::OnLine(const std::string& line)
{
printf("duude line >>%s<<\n", line.c_str());
}



// ==================================================================
