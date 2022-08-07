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
 * An instance of this class is created when a network client connects
 * to the server. It handles all socket read/write for that client.
 *
 * When a client connects to the server, the ServerSocket::handle_connection()
 * method is called in a new thread (and thus all socket reads for that
 * client occur in this thread.)
 *
 * This class has two pure-virtual methods: OnConnection() and OnLine().
 * The OnConnection() method is called once, when the reader thread is
 * first entered.  The OnLine() method is called whenever a new-line
 * delimited bit of utf-8 text data arrives on the socket.
 *
 * Users of this class need only implement these two methods.
 *
 * This class exists only to hide the ugliness of boost:asio.
 * Some day in the future, this should be re-written to avoid using
 * boost! But for now, it's stable debugged and it works.
 */
class ServerSocket
{
private:
    boost::asio::ip::tcp::socket* _socket;

    // A count of the number of concurrent open sockets. This is used
    // to limit the number of connections to the server, so that it
    // doesn't crash with a `accept: Too many open files` error.
    static unsigned int _max_open_sockets;
    static volatile unsigned int _num_open_sockets;
    static std::mutex _max_mtx;
    static std::condition_variable _max_cv;

    // A count of the number of times the max condition was reached.
    static size_t _num_open_stalls;

protected:
    /**
     * Connection callback: called whenever a new connection arrives
     */
    virtual void OnConnection(void) = 0;

    /**
     * Callback: called when a client has sent us a line of text.
     */
    virtual void OnLine (const std::string&) = 0;

    /// If true, then decode websocket frames.
    bool _decode_frames;

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
    void set_framing_mode(void) { _decode_frames = true; }

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

    /**
     * Status reporting API.
     */
    static void set_max_open_sockets(unsigned int m) { _max_open_sockets = m; }
    static unsigned int get_max_open_sockets() { return _max_open_sockets; }
    static unsigned int get_num_open_sockets() { return _num_open_sockets; }
    static size_t get_num_open_stalls() { return _num_open_stalls; }
}; // class

/** @}*/
}  // namespace

#endif // _OPENCOG_SERVER_SOCKET_H
