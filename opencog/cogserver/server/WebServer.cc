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
	_first_line(false),
	_http_handshake(false),
	_websock_handshake(false),
	_websock_open(false)
{
printf("duuude web ctor this=%p\n", this);
}

WebServer::~WebServer()
{
printf("duuude web dtor this=%p\n", this);
}

// ==================================================================

static std::string base64_encode(unsigned char* buf, int len)
{
	std::string out;

	unsigned int val = 0;
	int valb = -6;
	for (int i=0; i<len; i++)
	{
		unsigned char c = buf[i];
		val = (val << 8) + c;
		valb += 8;
		while (valb >= 0)
		{
			out.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[(val>>valb)&0x3F]);
			valb -= 6;
		}
	}
	if (valb > -6) out.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[((val<<8)>>(valb+8))&0x3F]);
	while (out.size()%4) out.push_back('=');
	return out;
}

// ==================================================================

void WebServer::OnConnection(void)
{
printf("duude connect\n");
}

void WebServer::OnLine(const std::string& line)
{
printf("duude line %d %d %d %p >>%s<<\n", _http_handshake,
_websock_handshake, _websock_open, this, line.c_str());
if(0x7f < (unsigned int) line[0]) {
for(size_t i=0; i< line.size(); i++) {
printf("%x ", (unsigned int) line[i]);
}
printf("\n");
}

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

	// If we are here, then the full HTTP header was received.
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

	// If we are here, we've received an HTTP header, and it
	// as a WebSocket header. Do the websocket reply, if we
	// haven't done so yet.
	if (not _websock_open)
	{
		_websock_open = true;
		_webkey += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

		unsigned char hash[SHA_DIGEST_LENGTH]; // == 20
		memset(hash, 0, SHA_DIGEST_LENGTH);
		SHA1((const unsigned char*) _webkey.c_str(), _webkey.size(), hash);
		std::string b64hash = base64_encode(hash, SHA_DIGEST_LENGTH);

		std::string response =
			"HTTP/1.1 101 Switching Protocols\r\n"
			"Upgrade: websocket\r\n"
			"Connection: Upgrade\r\n"
			"Sec-WebSocket-Accept: ";
		response += b64hash;
		response +=
			"\r\n"
			"\r\n";

printf("duuuude %p websoc response=\n%s\n", this, response.c_str());
		Send(response);
		return;
	}
printf("duuuude websock %p is bidi\n", this);

	Send("yeah baby\n");
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
