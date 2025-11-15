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
#include <list>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#include <asio.hpp>
#include <opencog/network/ServerSocket.h>
#include <opencog/network/SocketManager.h>

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
 * (server sockets bind to all interfaces in dual-stack mode, accepting both
 * IPv4 and IPv6 connections). Thus, server sockets are identified/selected
 * by the port they bind to.
 */
class NetworkServer
{
protected:
    std::string _name;
    short _port;
    std::atomic_bool _running;
    asio::io_service _io_service;
    asio::ip::tcp::acceptor _acceptor;
    std::thread* _listener_thread;

    /** Track all handler threads for proper cleanup */
    std::mutex _handler_threads_mtx;
    std::list<std::thread*> _handler_threads;

    /** Socket manager for tracking and managing all sockets (shared across all servers) */
    SocketManager* _socket_manager;

    /** The network server's main listener thread.  */
    void listen();
    ServerSocket* (*_getServer)(SocketManager*);

    /** monitoring stats */
    time_t _start_time;
    time_t _last_connect;
    size_t _nconnections;

public:

    /**
     * Starts the NetworkServer in a new thread.
     * The socket listen happens in the new thread.
     */
    NetworkServer(unsigned short port, const char* name, SocketManager* mgr);
    ~NetworkServer();

    /** Start and stop the server */
    void run(ServerSocket* (*)(SocketManager*));
    void stop();

    /** Get the socket manager */
    SocketManager* get_socket_manager() { return _socket_manager; }

    /** Get server start time for stats display */
    time_t get_start_time() const { return _start_time; }
    const char* get_name() const { return _name.c_str(); }

    /** Get the port this server is listening on */
    short getPort() const { return _port; }
}; // class

/** @}*/
}  // namespace

#endif // _OPENCOG_SIMPLE_NETWORK_SERVER_H
