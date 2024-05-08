/*
 * opencog/network/NetworkServer.h
 *
 * Copyright (C) 2002-2007 Novamente LLC
 * Written by Andre Senna <senna@vettalabs.com>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef _OPENCOG_SIMPLE_NETWORK_SERVER_H
#define _OPENCOG_SIMPLE_NETWORK_SERVER_H

#include <atomic>
#include <queue>
#include <string>
#include <thread>

#include <boost/asio.hpp>
#include <opencog/network/ServerSocket.h>

namespace opencog
{
/** \addtogroup grp_server
 *  @{
 */

/**
 * This class implements the entity responsible for managing the
 * server's network server.
 *
 * The network server runs on its own thread (thus freeing the server's
 * main loop to deal with requests only). It may be enabled/disabled at
 * will so that a server may run in networkless mode if desired.
 *
 * The network server supports only one server socket. Client applications
 * should use the 'start' method to start listening to a port.
 * Currently, the network server doesn't
 * support selecting the network interface that the server socket will bind to
 * (every server sockets binds to 0.0.0.0, i.e., all interfaces). Thus,
 * server sockets are identified/selected by the port they bind to.
 */
class NetworkServer
{
protected:
    std::string _name;
    short _port;
    std::atomic_bool _running;
    boost::asio::io_service _io_service;
    boost::asio::ip::tcp::acceptor _acceptor;
    std::thread* _listener_thread;

    /** The network server's main listener thread.  */
    void listen();
    ServerSocket* (*_getServer)(void);

    /** monitoring stats */
    time_t _start_time;
    time_t _last_connect;
    size_t _nconnections;

public:

    /**
     * Starts the NetworkServer in a new thread.
     * The socket listen happens in the new thread.
     */
    NetworkServer(unsigned short port, const char* name);
    ~NetworkServer();

    /** Start and stop the server */
    void run(ServerSocket* (*)(void));
    void stop();

    /** Print network stats in human-readable tabular form */
    std::string display_stats(int nlines = -1);
}; // class

/** @}*/
}  // namespace

#endif // _OPENCOG_SIMPLE_NETWORK_SERVER_H
