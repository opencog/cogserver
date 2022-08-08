/*
 * opencog/cogserver/server/WebServer.cc
 *
 * Copyright (C) 2022 Linas Vepstas
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifdef HAVE_OPENSSL

#include <string>
#include <openssl/sha.h>

#include <opencog/util/exceptions.h>
#include <opencog/util/Logger.h>
#include <opencog/util/misc.h>

#include <opencog/cogserver/server/CogServer.h>
#include <opencog/cogserver/server/WebServer.h>

using namespace opencog;

WebServer::WebServer(void) :
	_request(nullptr)
{
}

WebServer::~WebServer()
{
}

// ==================================================================

// Called before any data is sent/received.
void WebServer::OnConnection(void)
{
	// If the the socket didn't connect as a websocet, then just
	// report the stats as an HTML page.
	if (not _got_websock_header)
	{
		Send (html_stats());
		throw SilentException();
	}

	// We expect the URL to have the form /json or /scm or
	// whatever, and, stripping away the leading slash, it
	// should be one of the supported comands.
	std::string cmdName = _url.substr(1);
	CogServer& cs = cogserver();
	_request = cs.createRequest(cmdName);

	// Reject URL's we don't know about.
	if (nullptr == _request)
	{
		logger().info("[WebServer] Unsupported request %s", _url.c_str());
		Send("HTTP/1.1 404 Not Found\r\n"
			"Server: CogServer\r\n"
			"Content-Type: text/plain\r\n"
			"\r\n"
			"404 Not Found\n"
			"The Cogserver doesn't know about " + _url + "\n");
		throw SilentException();
	}

	logger().info("Opened WebSocket %s Shell", cmdName);
}

// Called for each newline-terminated line received.
void WebServer::OnLine(const std::string& line)
{
	if (_request)
	{
		// Use the request mechanism to get a fully configured
		// shell. This is a hang-over from the telnet interfaces,
		// where input strings become Requests, which, when executed
		// are looked up in the module system, passed to the correct
		// module, and then configured to send replies on this socket.
		// It works, so don't mess with it.
		std::list<std::string> params;
		params.push_back("hush");
		_request->setParameters(params);
		_request->set_console(this);
		_request->execute();
		get(); // Incremenet use count.
		delete _request;
		_request = nullptr;

		// Disable line discipline
		_shell->discipline(false);
	}
	_shell->eval(line);
}


// ==================================================================

std::string WebServer::html_stats(void)
{
	std::string response =
		"HTTP/1.1 200 OK\r\n"
		"Server: CogServer\r\n"
		"Content-Type: text/html\r\n"
		"\r\n"
		"<!DOCTYPE html>"
		"<html>"
		"<head><title>CogServer Stats</title>"
		"  <meta charset=\"UTF-8\"></head>"
		"<body>"
		"<h2>CogServer Stats</h2>"
		"<pre>\n";
	response += cogserver().display_stats();
	response +=
		"</pre>"
		"<h2>Stats Legend</h2>"
		"<pre>";
	response += CogServer::stats_legend();
	response += "</pre></body></html>";

	return response;
}

#endif // HAVE_OPENSSL
// ==================================================================
