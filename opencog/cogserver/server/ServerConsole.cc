/*
 * opencog/cogserver/server/ServerConsole.cc
 *
 * Copyright (C) 2002-2007 Novamente LLC
 * Written by Andre Senna <senna@vettalabs.com>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

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
#define IAC 0xff  // Telnet Interpret As Command
#define IP 0xf4   // Telnet IP Interrupt Process
#define AO 0xf5   // Telnet AO Abort Output
#define EL 0xf8   // Telnet EL Erase Line
#define WILL 0xfb // Telnet WILL
#define SB 0xfa   // Telnet SB subnegotiation start
#define SE 0xf0   // Telnet SE subnegotiation end
#define DO 0xfd   // Telnet DO
#define DONT 0xfe   // Telnet DONT
#define CHARSET 0x2a // Telnet RFC 2066 charset

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

    // Hmm. Looks like most telnet agents respond with an
    // IAC WONT CHARSET IAC DONT CHARSET
    // Any case, just ignore CHARSET RFC 2066 negotiation
    if (IAC == (line[0] & 0xff) and CHARSET == (line[2] & 0xff)) {
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

    // Parse command line
    std::list<std::string> params;
    tokenize(line, std::back_inserter(params), " \t\v\f");
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

void ServerConsole::OnRequestComplete()
{
    logger().debug("[ServerConsole] OnRequestComplete");

    // Shells will send their own prompt
    if (nullptr == _shell) sendPrompt();
}

void ServerConsole::Exit()
{
    logger().debug("[ServerConsole] ExecuteExitRequest");
    SetCloseAndDelete();
}

void ServerConsole::SendResult(const std::string& res)
{
    Send(res);
}
