/*
 * opencog/network/ServerSocket.h
 *
 * Copyright (C) 2002-2007 Novamente LLC
 * Written by Welter Luigi <welter@vettalabs.com>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef _OPENCOG_SERVER_SOCKET_H
#define _OPENCOG_SERVER_SOCKET_H

#include <boost/asio.hpp>

namespace opencog
{
/** \addtogroup grp_server
 *  @{
 */

/**
 * This class defines the minimal set of methods a server socket must
 * have to handle the primary interface of the server.
 *
 * Each ServerSocket supports a client that connects to the cog server.
 */
class ServerSocket
{
private:
    boost::asio::ip::tcp::socket* _socket;

protected:
    /**
     * Connection callback: called whenever a new connection arrives
     */
    virtual void OnConnection(void) = 0;

    /**
     * Callback: called when a client has sent us a line of text.
     */
    virtual void OnLine (const std::string&) = 0;

public:
    ServerSocket(void);
    virtual ~ServerSocket();

    void set_connection(boost::asio::ip::tcp::socket*);
    void handle_connection(void);

    /**
     * Sends data to the client
     */
    void Send(const std::string&);

    /**
     * Close this socket
     */
    void SetCloseAndDelete(void);
}; // class

/** @}*/
}  // namespace

#endif // _OPENCOG_SERVER_SOCKET_H
