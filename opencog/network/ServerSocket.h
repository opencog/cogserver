/*
 * opencog/network/ServerSocket.h
 *
 * Copyright (C) 2002-2007 Novamente LLC
 * Written by Welter Luigi <welter@vettalabs.com>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef _OPENCOG_SERVER_SOCKET_H
#define _OPENCOG_SERVER_SOCKET_H

#include <atomic>
#include <pthread.h>
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

    /**
     * Report human-readable stats for this socket.
     */
    time_t _start_time;
    pid_t _tid;    // OS-dependent thread ID.
    pthread_t _pth;
    const char* _status; // "start" or "run" or "close"
    time_t _last_activity;
    size_t _line_count;

    virtual std::string connection_header(void);
    virtual std::string connection_stats(void);
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

    /**
     * Return a human-readable table of socket statistics.
     * Used for monitoring the server state.
     * Loops over all active sockets.
     */
    static std::string display_stats(void);

    /** Attempt top close half-open sockets, if any. */
    static void half_ping(void);

    /** Attempt to kill the indicated thread. */
    static bool kill(pid_t);

    /** Total line count, handled by all sockets, ever. */
    static std::atomic_size_t total_line_count;
}; // class

/** @}*/
}  // namespace

#endif // _OPENCOG_SERVER_SOCKET_H
