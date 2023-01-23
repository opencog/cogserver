/*
 * opencog/cogserver/server/ServerConsole.h
 *
 * Copyright (C) 2002-2007 Novamente LLC
 * Written by Andre Senna <senna@vettalabs.com>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef _OPENCOG_SERVER_CONSOLE_H
#define _OPENCOG_SERVER_CONSOLE_H

#include <condition_variable>
#include <mutex>
#include <string>

#include <opencog/network/ConsoleSocket.h>

namespace opencog
{
/** \addtogroup grp_server
 *  @{
 */

/**
 * This class implements the ConsoleSocket that handles the primary
 * interface of the cogserver: the plain text command line.
 *
 * There may be multiple instances of ConsoleSocket to support multiple
 * simultaneous clients. This is done by creating a separate thread and
 * dispatching a client socket for each client that connects to the
 * server socket.
 */
class ServerConsole : public ConsoleSocket
{
private:
    static std::string _prompt;

protected:
    bool handle_telnet_iac(const std::string&);

    /**
     * Connection callback: called whenever a new connection arrives
     */
    void OnConnection(void);

    /**
     * OnLine callback: called when a new command/request is received
     * from the client. It parses the command line by splitting it into
     * space-separated tokens.
     *
     * The first token is used as the request's id/name and the
     * remaining tokens are used as the request's parameters. The
     * request name is used to identify the request type.
     *
     * If the request type is not found, we execute the 'HelpRequest',
     * which will return a useful message to the client.
     *
     * If the request class is found, we instantiate a new request,
     * set its parameters and push it to the cogserver's request queue.
     */
    void OnLine(const std::string&);

public:
    /**
     * Ctor. Defines the socket's mime-type as 'text/plain' and then
     * configures the Socket to use line protocol.
     */
    ServerConsole(void);
    ~ServerConsole();

    /**
     * Send a prompt string.
     */
    void sendPrompt();

}; // class

/** @}*/
}  // namespace

#endif // _OPENCOG_SERVER_CONSOLE_H
