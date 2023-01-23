/*
 * opencog/cogserver/server/ServerConsole.cc
 *
 * Copyright (C) 2002-2007 Novamente LLC
 * Written by Andre Senna <senna@vettalabs.com>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <time.h>

#include <string>

#include <opencog/util/Config.h>
#include <opencog/util/Logger.h>
#include <opencog/util/misc.h>

#include <opencog/cogserver/server/CogServer.h>
#include <opencog/cogserver/server/ServerConsole.h>
#include <opencog/cogserver/server/Request.h>

using namespace opencog;

std::string ServerConsole::_prompt;

ServerConsole::ServerConsole(void)
{
    if (nullptr == &config()) {
        _prompt = "[0;32mopencog[1;32m> [0m";
    } else {
        // Prompt with ANSI color codes, if possible.
        if (config().get_bool("ANSI_ENABLED", true))
            _prompt = config().get("ANSI_PROMPT", "[0;32mopencog[1;32m> [0m");
        else
            _prompt = config().get("PROMPT", "opencog> ");
    }
}

ServerConsole::~ServerConsole()
{
}

// Some random RFC 854 characters
#define IAC  0xff // Telnet Interpret As Command
#define TEOF 0xec // Telnet EOF      RFC 1184 linemode EOF   236
#define SUSP 0xed // Telnet suspend  RFC 1184 linemode SUSP  237
#define ABRT 0xee // Telnet abort    RFC 1184 linemode ABORT 238
#define SE   0xf0 // Telnet SE End of subnegotiation
#define NOP  0xf1 // Telnet NOP no-op
#define MARK 0xf2 // Telnet Data Mark
#define BRK  0xf3 // Telnet break
#define IP   0xf4 // Telnet IP Interrupt Process
#define AO   0xf5 // Telnet AO Abort Output
#define AYT  0xf6 // Telnet AYT Are You There
#define EC   0xf7 // Telnet EC Erase Character
#define EL   0xf8 // Telnet EL Erase Line
#define GA   0xf9 // Telnet GA Go ahead
#define SB   0xfa // Telnet SB Begin subnegotiation
#define WILL 0xfb // Telnet WILL
#define WONT 0xfc // Telnet WONT
#define DO   0xfd // Telnet DO
#define DONT 0xfe // Telnet DONT

#define TRANSMIT_BINARY   0  // Telnet RFC 856 8-bit-clean binary transmission
#define RFC_ECHO          1  // Telnet RFC 857 ECHO option
#define SUPPRESS_GO_AHEAD 3  // Telnet RFC 858 supporess go ahead
#define TIMING_MARK       6  // Telnet RFC 860 timing mark
#define WINSIZE          31  // Telnet RFC 1073 window size
#define SPEED            32  // Telnet RFC 1079 terminal speed
#define LINEMODE         34  // Telnet RFC 1116 linemode
#define CHARSET        0x2A  // Telnet RFC 2066

/// Return true if the Telnet IAC command was rcognized and handled.
bool ServerConsole::handle_telnet_iac(const std::string& line)
{
	// Hmm. Most telnet agents respond with an
	// IAC WONT CHARSET IAC DONT CHARSET
	// Ignore CHARSET RFC 2066 negotiation

	int sz = line.size();
	int i = 0;
	while (i < sz)
	{
		unsigned char iac = line[i];
		if (IAC != iac) return false;
		i++; if (sz <= i) return false;
		unsigned char c = line[i];
		i++; if (sz <= i) return false;
		unsigned char a = line[i];
		i++;

		logger().debug("[ServerConsole] %d Received telnet IAC %d %d", i, c, a);

		// Actually, ignore all WONT & DONT.
		if (WONT == c) continue;

		if (DONT == c)
		{
			continue;
		}

		if (DO == c)
		{
			// If telnet ever tries to go into character mode,
			// it will send us SUPPRESS-GO-AHEAD and ECHO. Try to
			// stop that; we don't want to effing fiddle with that.
			if (SUPPRESS_GO_AHEAD == a)
			{
				unsigned char ok[] = {IAC, WONT, SUPPRESS_GO_AHEAD, 0};
				Send((const char *) ok);
				continue;
			}
			if (RFC_ECHO == a)
			{
				unsigned char ok[] = {IAC, WONT, RFC_ECHO, '\n', 0};
				Send((const char *) ok);
				continue;
			}
			// logger().debug("[ServerConsole] Sending IAC WONT %d", a);
			// unsigned char ok[] = {IAC, WONT, a, '\n', 0};
			// Send((const char *) ok);
			logger().debug("[ServerConsole] Ignoring IAC DO %d", a);
			continue;
		}

		if (WILL == c)
		{
			// Refuse to perform sub-negotation when
			// IAC WILL LINEMODE is sent by the telnet client.
			if (LINEMODE == a)
			{
				unsigned char ok[] = {IAC, DONT, LINEMODE, '\n', 0};
				Send((const char *) ok);
				continue;
			}

			// Ignore anything else.
			logger().debug("[ServerConsole] Ignoring telnet IAC WILL %d", a);
			continue;
		}
	}

	return true;
}

void ServerConsole::OnConnection()
{
    logger().debug("[ServerConsole] OnConnection");

#ifdef NOT_RIGHT_NOW
    // Crude attempt to negotiate for a utf-8 clean channel.
    // Using RFC 2066 protocols.  Not robust.  We're just praying
    // for non-garbled UTF-8 goodness, here.

    // Anyway, this won't work for netcat, socat, because they'll
    // just pass all this crap back to the user, and we don't want
    // that.  I'm not sure how to tell if we're talking to a true
    // RFC telnet. Maybe we need to listen to a different port?
    char utf_plz[20];
    utf_plz[0] = IAC;
    utf_plz[1] = WILL;
    utf_plz[2] = CHARSET;
    utf_plz[3] = 0;
    Send(utf_plz);
    utf_plz[1] = DO;
    Send(utf_plz);

    utf_plz[0] = IAC;
    utf_plz[1] = SB;
    utf_plz[2] = CHARSET;
    utf_plz[3] = '\02';
    utf_plz[4] = 'U';
    utf_plz[5] = 'T';
    utf_plz[6] = 'F';
    utf_plz[7] = '-';
    utf_plz[8] = '8';
    utf_plz[9] = IAC;
    utf_plz[10] = SE;
    utf_plz[11] = 0;
    Send(utf_plz);
#endif

    sendPrompt();
}

void ServerConsole::sendPrompt()
{
    // Hush prompts are empty. Don't call.
    if (0 < _prompt.size())
        Send(_prompt);
}


/// Parse command line. Quotes are stripped.
/// XXX escaped quotes are not handled correctly. FIXME.
/// This passes over quotes embeded in the middle strings.
/// And that OK, because what the heck did you want to happen?
static std::list<std::string> simple_tokenize(const std::string& line)
{
    std::list<std::string> params;
    size_t pos = 0;
    size_t len = line.size();
    while (pos<len) {
        // Gather up everything until the next quote.
        if ('\"' == line[pos]) {
            std::string buff;
            pos++; // skip over opening quote
            while ('\"' != line[pos] and pos < len) {
               buff.push_back(line[pos]);
               pos++;
            }
            pos++; // skip over closing quote
            params.push_back(buff);
            if (len <= pos) break;
            continue;
        }

        // Gather up everything until the next blank space.
        if (' ' != line[pos]) {
            std::string buff;
            while (' ' != line[pos] and pos < len) {
               buff.push_back(line[pos]);
               pos++;
            }
            params.push_back(buff);
            if (len <= pos) break;
            continue;
        }

        // Skip over blank spaces.
        if (' ' == line[pos]) {
            while (' ' == line[pos] and pos < len) pos++;
            if (len <= pos) break;
            continue;
        }
    }
    return params;
}

void ServerConsole::OnLine(const std::string& line)
{
    // If a shell processor has been designated, then defer all
    // processing to the shell.  In particular, avoid as much overhead
    // as possible, since the shell needs to be able to handle a
    // high-speed data feed with as little getting in the way as
    // possible.
    if (_shell) {
        _shell->eval(line);
        return;
    }

    // Look for telnet stuff, and process it.
    if (IAC == (line[0] & 0xff)
        and line.size() < 40
        and handle_telnet_iac(line))
    {
       return;
    }

    // If the command starts with an open-paren, or a semi-colon, assume
    // its a scheme command. Pop into the scheme shell, and try again.
    if (line[0] == '(' or line[0] == ';')
    {
        OnLine("scm");

        // Re-issue the command, but only if we sucessfully got a shell.
        // (We might not get a shell if scheme is not installed.)
        if (_shell) {
            OnLine(line);
            return;
        }
    }

    logger().debug("[ServerConsole] OnLine [%s]", line.c_str());

    // Parse command line. Quotes are stripped.
    // tokenize(line, std::back_inserter(params), " \t\v\f");
    std::list<std::string> params = simple_tokenize(line);

    logger().debug("params.size(): %d", params.size());
    if (params.empty()) {
        // return on empty/blank line
        sendPrompt();
        return;
    }

    std::string cmdName = params.front();
    params.pop_front();

    CogServer& cs = cogserver();
    Request* request = cs.createRequest(cmdName);

    // Command not found.
    if (nullptr == request)
    {
        char msg[256];
        snprintf(msg, 256, "command \"%s\" not found\n", cmdName.c_str());
        logger().debug("%s", msg);
        Send(msg);

        // Try to send "help" command response
        request = cs.createRequest("help");
        if (nullptr == request)
        {
            // no help request; just terminate the request
            sendPrompt();
            return;
        }
    }

    request->set_console(this);
    request->setParameters(params);
    bool is_shell = request->isShell();

    // Add the command to the processing queue.
    // Caution: after the pushRequest, the request might be executed
    // and then deleted in a different thread. It must NOT be accessed
    // after the push!
    cs.pushRequest(request);

    if (is_shell)
    {
        logger().debug("ServerConsole::OnLine() request \"%s\" is a shell",
                       line.c_str());

        // Force a drain of the request queue, because we *must* enter
        // shell mode before handling any additional input from the
        // socket (since all subsequent input will be for the new shell,
        // not for the cogserver command processor).
        cs.processRequests();
    }
}

// ==================================================================
