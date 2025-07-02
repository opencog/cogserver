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
	// Log this as 'debug' not 'info', due to nuisance websocket
	// traffic coming from systemd which pings it every 5 seconds.
	// Or maybe its the gnome dbus or soething like that.
	logger().debug("Closed WebSocket Shell");
}

// ==================================================================

// Called before any data is sent/received.
void WebServer::OnConnection(void)
{
	if (0 == _url.compare("/favicon.ico"))
	{
		Send(favicon());
		throw SilentException();
	}
	if (0 == _url.compare("/stats"))
	{
		Send(html_stats());
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

	logger().info("Opened Http Socket %s Shell", cmdName.c_str());
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
		"<h2>Loaded Modules</h2>"
		"<pre>\n";
	response += cogserver().listModules();
	response +=
		"</pre>"
		"<h2>CogServer Stats</h2>"
		"<pre>\n";
	response += cogserver().display_web_stats();
	response +=
		"</pre>"
		"<h2>CogServer Stats Legend</h2>"
		"<pre>";
	response += CogServer::stats_legend();
	response += "</pre></body></html>";

	return response;
}

// ==================================================================

/// Given an input in base64, return the raw binary, stuffed into
/// a string.
// Found code blob on stackexchange from user Manuel Martinez.
static std::string base64_decode(const std::string &in)
{
	std::string out;

	std::vector<int> T(256,-1);
	for (int i=0; i<64; i++) T["ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[i]] = i;

	int val=0, valb=-8;
	for (unsigned char c : in)
	{
		if (T[c] == -1) break;
		val = (val << 6) + T[c];
		valb += 6;
		if (valb >= 0)
		{
			out.push_back(char((val>>valb) & 0xFF));
			valb -= 8;
		}
	}
	return out;
}

// ==================================================================

/// Return an HTTP response holding the opencog favicon.ico image.
std::string WebServer::favicon(void)
{
	// I do not want to open and read a file; so we're going to
	// just insert this into the source code. Which means it cannot
	// be a binary blob. So we insert base64 into the code.
	// Now, it would be great if Firefox/Mozilla accepted
	// Content-Transfer-Encoding: base64 but it doesn't, so we
	// convert the thing into binary, and send that.
	std::string bicon =
#include "favicon.ico.base64"
	;
	std::string icon = base64_decode(bicon);

	std::string response =
		"HTTP/1.1 200 OK\r\n"
		"Server: CogServer\r\n"
		"Content-Length: ";

	char buf[20];
	snprintf(buf, 20, "%lu", icon.size());
	response += buf;
	response += "\r\n"
		"Content-Type: image/vnd.microsoft.icon\r\n"
		"\r\n";

	response += icon;

	return response;
}

#endif // HAVE_OPENSSL
// ==================================================================
