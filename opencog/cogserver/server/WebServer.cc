/*
 * opencog/cogserver/server/WebServer.cc
 *
 * Copyright (C) 2022 Linas Vepstas
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifdef HAVE_OPENSSL

#include <cstring>
#include <string>
#include <openssl/sha.h>

#include <opencog/util/exceptions.h>
#include <opencog/util/Logger.h>
#include <opencog/util/misc.h>

#include <opencog/cogserver/server/CogServer.h>
#include <opencog/cogserver/server/PageServer.h>
#include <opencog/cogserver/server/WebServer.h>
#include <opencog/eval/GenericEval.h>

using namespace opencog;

WebServer::WebServer(CogServer& cs, SocketManager* mgr) :
	ConsoleSocket(mgr),
	_cserver(cs),
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

#ifdef HAVE_MCP
	// Handle OAuth discovery endpoints for MCP
	if (0 == _url.compare("/.well-known/oauth-protected-resource"))
	{
		Send(oauth_protected_resource());
		throw SilentException();
	}
	if (0 == _url.compare("/.well-known/oauth-authorization-server"))
	{
		Send(oauth_authorization_server());
		throw SilentException();
	}
	// Handle the /register endpoint that Claude is looking for
	if (0 == _url.compare("/register"))
	{
		Send(oauth_register_not_required());
		throw SilentException();
	}
#endif

	// We expect the URL to have the form /json or /scm or
	// whatever, and, stripping away the leading slash, it
	// should be one of the supported commands.
	std::string cmdName = _url.substr(1);
	_request = _cserver.createRequest(cmdName);

	// Reject URL's we don't know about.
	// Try to serve static pages from /usr/local/share/cogserver
	if (nullptr == _request)
	{
		logger().info("[WebServer] Request not found, trying PageServer for %s", _url.c_str());
		std::string response = PageServer::serve(_url);
		Send(response);
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

	// WebSocket connections use normal threaded evaluation
	if (_got_websock_header)
	{
		_shell->eval(line);
		return;
	}

	// For non-WebSocket HTTP connections, don't use the shell's threaded evaluation
	// Instead, get the evaluator directly and use it synchronously
	GenericEval* eval = _shell->get_evaluator();

	// Start evaluation
	eval->begin_eval();

	// Evaluate the expression
	eval->eval_expr(line);

	// Collect all results
	std::string result;
	std::string chunk;
	do {
		chunk = eval->poll_result();
		result += chunk;
	} while (!chunk.empty());

	// Send with appropriate HTTP headers
	if (!result.empty())
	{
		// Determine content type based on shell type
		std::string content_type = "text/plain";
		if (strcmp(_shell->_name, "mcp") == 0 || strcmp(_shell->_name, "json") == 0)
			content_type = "application/json";

		SendWithHeader(result, content_type);
	}
}


// ==================================================================

std::string WebServer::html_stats(void)
{
	std::string response =
		"HTTP/1.1 200 OK\r\n"
		"Server: CogServer\r\n"
		"Content-Type: text/html\r\n"
		"\r\n"
		"<!DOCTYPE html>\n"
		"<html lang=\"en\">\n"
		"<head>\n"
		"  <meta charset=\"UTF-8\">\n"
		"  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
		"  <title>CogServer Status</title>\n"
		"  <style>\n"
		"    body {\n"
		"      font-family: monospace;\n"
		"      margin: 20px;\n"
		"      line-height: 1.6;\n"
		"      background: #fafafa;\n"
		"    }\n"
		"    h1 {\n"
		"      border-bottom: 2px solid #333;\n"
		"      padding-bottom: 10px;\n"
		"      color: #333;\n"
		"    }\n"
		"    h2 {\n"
		"      border-bottom: 1px solid #ccc;\n"
		"      padding-bottom: 5px;\n"
		"      margin-top: 30px;\n"
		"      color: #444;\n"
		"    }\n"
		"    pre {\n"
		"      background: #f0f0f0;\n"
		"      padding: 15px;\n"
		"      border: 1px solid #ccc;\n"
		"      border-radius: 4px;\n"
		"      overflow-x: auto;\n"
		"      font-size: 14px;\n"
		"      line-height: 1.4;\n"
		"    }\n"
		"    a {\n"
		"      color: #007bff;\n"
		"      text-decoration: none;\n"
		"    }\n"
		"    a:hover {\n"
		"      text-decoration: underline;\n"
		"    }\n"
		"  </style>\n"
		"</head>\n"
		"<body>\n"
		"  <h1>CogServer Status</h1>\n"
		"  <h2>Loaded Modules</h2>\n"
		"  <pre>\n";
	response += _cserver.listModules();
	response +=
		"</pre>\n"
		"  <h2>Connection Statistics</h2>\n"
		"  <pre>\n";
	response += _cserver.display_web_stats();
	response +=
		"</pre>\n"
		"  <h2>Connection Stats Legend</h2>\n"
		"  <pre>";
	response += CogServer::stats_legend();
	response +=
		"</pre>\n"
		"</body>\n"
		"</html>";

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

#ifdef HAVE_MCP
// ==================================================================
// OAuth discovery endpoints for MCP

/// Return OAuth protected resource metadata indicating no authentication required
std::string WebServer::oauth_protected_resource(void)
{
	// This indicates that no authentication is required
	// by not specifying any authorization_servers
	std::string json_body = "{}";

	std::string response =
		"HTTP/1.1 200 OK\r\n"
		"Server: CogServer\r\n"
		"Content-Type: application/json\r\n"
		"Content-Length: ";

	char buf[20];
	snprintf(buf, 20, "%lu", json_body.size());
	response += buf;
	response += "\r\n\r\n";
	response += json_body;

	return response;
}

/// Return OAuth authorization server metadata indicating no authentication
std::string WebServer::oauth_authorization_server(void)
{
	// Build the issuer URL from the Host header if available
	std::string issuer_url = "http://";
	if (!_host_header.empty()) {
		issuer_url += _host_header;
	} else {
		// Fallback to localhost with actual port if no Host header was provided
		short port = _cserver.getWebServerPort();
		issuer_url += "localhost:" + std::to_string(port);
	}

	// Return minimal OAuth metadata indicating no authentication required
	std::string json_body = "{\"issuer\":\"" + issuer_url + "\"}";

	std::string response =
		"HTTP/1.1 200 OK\r\n"
		"Server: CogServer\r\n"
		"Content-Type: application/json\r\n"
		"Content-Length: ";

	char buf[20];
	snprintf(buf, 20, "%lu", json_body.size());
	response += buf;
	response += "\r\n\r\n";
	response += json_body;

	return response;
}

/// Return a response indicating registration is not required
std::string WebServer::oauth_register_not_required(void)
{
	// Return a response indicating that registration is not needed
	std::string json_body = "{\"error\":\"registration_not_supported\",\"error_description\":\"This MCP server does not require OAuth registration\"}";

	std::string response =
		"HTTP/1.1 400 Bad Request\r\n"
		"Server: CogServer\r\n"
		"Content-Type: application/json\r\n"
		"Content-Length: ";

	char buf[20];
	snprintf(buf, 20, "%lu", json_body.size());
	response += buf;
	response += "\r\n\r\n";
	response += json_body;

	return response;
}
#endif // HAVE_MCP

// Send with HTTP headers for shell output
void WebServer::SendWithHeader(const std::string& msg, const std::string& content_type)
{
	// Build HTTP response with Content-Length
	std::string response = "HTTP/1.1 200 OK\r\n";
	response += "Server: CogServer\r\n";
	response += "Content-Type: ";
	response += content_type;
	response += "\r\n";
	response += "Content-Length: ";
	char buf[20];
	snprintf(buf, 20, "%lu", msg.size());
	response += buf;
	response += "\r\n\r\n";
	response += msg;

	// Send the complete HTTP response
	Send(response);
}

#endif // HAVE_OPENSSL
// ==================================================================
