/*
 * opencog/cogserver/server/WebServer.cc
 *
 * Copyright (C) 2022 Linas Vepstas
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#if HAVE_OPENSSL

#include <string>
#include <openssl/sha.h>

#include <opencog/util/exceptions.h>
#include <opencog/util/Logger.h>
#include <opencog/util/misc.h>

#include <opencog/cogserver/server/CogServer.h>
#include <opencog/cogserver/server/WebServer.h>

using namespace opencog;

WebServer::WebServer(void) :
	_first_line(false),
	_http_handshake(false),
	_websock_handshake(false)
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
printf("duude line %d %d >>%s<<\n", _http_handshake, _websock_handshake, line.c_str());

	if (not _first_line)
	{
		_first_line = true;

		if (0 != line.compare(0, 4, "GET "))
		{
			Send("HTTP/1.1 501 Not Implemented\r\n"
				"Server: CogServer\r\n"
				"\r\n");
			throw SilentException();
		}
		_url = line.substr(4, line.find(" ", 4) - 4);
		return;
	}

	if (not _http_handshake and 0 == line.size())
	{
		_http_handshake = true;
	}
	if (not _http_handshake)
	{
		static const char* upg = "Upgrade: websocket";
		if (0 == line.compare(0, strlen(upg), upg))
			{ _websock_handshake = true; return; }

		static const char* key = "Sec-WebSocket-Key: ";
		if (0 == line.compare(0, strlen(key), key))
			{ _webkey = line.substr(strlen(key)); return; }

		// TODO validate
		// static const char* org = "Origin: ";

		return;
	}

	// If we are here, that the full HTTP header was received.
	// If it wasn't a websocket header, then just print stats.
	if (not _websock_handshake)
	{
		Send (html_stats());
		throw SilentException();
	}

#if 0
	// Check for supported protocols
	if (0 != _url.compare("/json"))
	{
		logger().info("[WebServer] Unsupported request %s", line.c_str());
		Send("HTTP/1.1 404 Not Found\r\n"
			"Server: CogServer\r\n"
			"Content-Type: text/plain\r\n"
			"\r\n"
			"404 Not Found\n"
			"Cogserver currently supports only /json\n");
		throw SilentException();
	}
#endif

printf("duude webkey=>>%s<<\n", _webkey.c_str());
	_webkey += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

#if 0

  const unsigned char str[] = "Original String";
  unsigned char hash[SHA_DIGEST_LENGTH]; // == 20
  SHA1(str, sizeof(str) - 1, hash);

#endif

	std::string response =
		"HTTP/1.1 101 Switching Protocols\r\n"
		"Upgrade: websocket\r\n"
		"Connection: Upgrade\r\n"
		"\r\n";
      // Sec-WebSocket-Accept: HSmrc0sMlYUkAGmm5OPpG2HaGWk=
      // Sec-WebSocket-Protocol: chat

	Send(response);
}

// ==================================================================

std::string WebServer::connection_stats(void)
{
	std::string rc = ServerSocket::connection_stats();
	rc += " 1 "; // use_count (fake)
	rc += "webs           ";
	return rc;
}

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
