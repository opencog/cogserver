/*
 * opencog/cogserver/server/WebServer.h
 *
 * Copyright (C) 2022 Linas Vepstas
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef _OPENCOG_WEB_SERVER_H
#define _OPENCOG_WEB_SERVER_H

#include <string>

#include <opencog/network/ConsoleSocket.h>
#include <opencog/cogserver/server/Request.h>

namespace opencog
{
/** \addtogroup grp_server
 *  @{
 */

/**
 * This class implements a super-simple WebSockets server.
 * The actual WebSockets protocol is handled in class ServerSocket.
 * The shell handling is in class ConsoleSocket.
 * All that we do is provide some glue.
 */
class WebServer : public ConsoleSocket
{
private:
	Request* _request;

protected:
	virtual void OnConnection(void);
	virtual void OnLine (const std::string&);

	std::string html_stats(void);
	std::string favicon(void);
public:
    WebServer(void);
    ~WebServer();

}; // class

/** @}*/
}  // namespace

#endif // _OPENCOG_WEB_SERVER_H
