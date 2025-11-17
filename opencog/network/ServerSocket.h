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
#include <asio.hpp>

namespace opencog
{

class SocketManager;

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
 * This class exists only to hide the ugliness of asio. Some day
 * in the future, this should be re-written to avoid using asio!
 * But for now, it's stable debugged and it works.
 */
class ServerSocket
{
private:
    // The actual socket on which data comes & goes.
    asio::ip::tcp::socket* _socket;

    // Socket manager handles registration and coordination
    SocketManager* _socket_manager;

    // Read a newline-delimited line of text from socket.
    std::string get_telnet_line(asio::streambuf&);

    // Read _content_length bytes
    std::string get_http_body(asio::streambuf&);

    // Send an asio buffer that has data in it.
    void Send(const asio::const_buffer&);

    // WebSocket state machine; unused in the telnet interface.
    bool _got_first_line;
    bool _got_http_header;
    bool _do_frame_io;
    std::string _webkey;
    void HandshakeLine(const std::string&);
    std::string get_websocket_data(void);
    std::string get_websocket_line(void);
    void send_websocket_pong(void);
    void send_websocket(const std::string&);

protected:
    // WebSocket stuff that users will be interested in.
    bool _is_http_socket;
    bool _got_websock_header;
    bool _is_mcp_socket;

    // KeepAlive connections will repeatedly send HTTP headers.
    bool _keep_alive;

    bool _in_barrier;

    size_t _content_length;

    std::string _url;
    std::string _host_header;  // Host header from HTTP request

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
    ServerSocket(SocketManager*);
    virtual ~ServerSocket();
    void act_as_http_socket(void) { _is_http_socket = true; }
    void act_as_mcp(void) { _is_mcp_socket = true; }

    // Access to socket manager for derived classes
    SocketManager* get_socket_manager() { return _socket_manager; }

    void set_connection(asio::ip::tcp::socket*);
    void handle_connection(void);

    /**
     * Send data to the client.
     */
    void Send(const std::string&);

    /**
     * Close this socket. Called from a thread other than
     * the one that is actually polling the socket.
     */
    void Exit(void);

    /** Total line count, handled by all sockets, ever. */
    static std::atomic_size_t total_line_count;

    /** Status string constants */
    static char START[6];
    static char BLOCK[6];
    static char IWAIT[6];
    static char QUING[6];
    static char BAR[6];
    static char DTOR[6];
    static char CLOSE[6];
    static char DOWN[6];

    // Expose frequently-needed socket manager operations
    // These delegate to the socket manager for this socket
    friend class SocketManager;
}; // class

/** @}*/
}  // namespace

#endif // _OPENCOG_SERVER_SOCKET_H
