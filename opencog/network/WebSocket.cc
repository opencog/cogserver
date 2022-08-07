/*
 * opencog/network/WebSocket.cc
 *
 * Copyright (C) 2022 Linas Vepstas
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifdef HAVE_OPENSSL

#include <string>
#include <openssl/sha.h>

#include <opencog/util/exceptions.h>
#include <opencog/util/Logger.h>

#include "ServerSocket.h"

using namespace opencog;

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

// Perform the webscokets handshake.
void ServerSocket::HandshakeLine(const std::string& line)
{
	// The very first HTTP line.
	if (not _got_first_line)
	{
		_got_first_line = true;

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

	// If the line-size is zero, then we've reached the end of
	// header sent by the client.
	if (not _got_http_header and 0 == line.size())
	{
		_got_http_header = true;
	}

	// Extract stuff from the header the client is sending us.
	if (not _got_http_header)
	{
		static const char* upg = "Upgrade: websocket";
		if (0 == line.compare(0, strlen(upg), upg))
			{ _got_websock_header = true; return; }

		static const char* key = "Sec-WebSocket-Key: ";
		if (0 == line.compare(0, strlen(key), key))
			{ _webkey = line.substr(strlen(key)); return; }

		return;
	}

	// If we are here, then the full HTTP header was received. This
	// is enough to get started: call the user's OnConnection()
	// method. The user is supposed to check two things:
	// (a) Do they like the URL in the header? If not, they
	//     should send some response e.g. 404 Not Found
	//     and then `throw SilentException()` to close the sock.
	// (b) Was an actual WebSocket negotiated? If not, then the
	//     user should send some response, e.g. 200 OK and some
	//     HTML, and then `throw SilentException()` to close the
	//     sock.
	OnConnection();

	// In case the user blew it above, we close the sock.
	if (not _got_websock_header)
		throw SilentException();

	// If we are here, we've received an HTTP header, and it
	// as a WebSocket header. Do the websocket reply.
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

	Send(response);

	// After this point, websockets will send frames.
	// Need to change the mode to work with frames.
	_do_frame_io = true;
}

#endif // HAVE_OPENSSL
// ==================================================================
