/*
 * opencog/server/NetworkServer.cc
 *
 * Copyright (C) 2002-2007 Novamente LLC
 * Copyright (C) 2008 by OpenCog Foundation
 * Written by Andre Senna <senna@vettalabs.com>
 *            Gustavo Gama <gama@vettalabs.com>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <netinet/tcp.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>

#include <boost/asio/ip/tcp.hpp>
#include <opencog/util/Logger.h>
#include <opencog/network/ServerSocket.h>
#include <opencog/network/ConsoleSocket.h>

#include "NetworkServer.h"

using namespace opencog;

NetworkServer::NetworkServer(unsigned short port, const char* name) :
    _name(name),
    _port(port),
    _running(false),
    _acceptor(_io_service,
        boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
{
    logger().debug("[NetworkServer] constructor for %s at %d", name, port);
    _start_time = time(nullptr);
    _last_connect = 0;
    _nconnections = 0;
}

NetworkServer::~NetworkServer()
{
    logger().debug("[NetworkServer] enter destructor for %s at %d",
                   _name.c_str(), _port);

    stop();

    logger().debug("[NetworkServer] all threads joined, exit destructor");
}

void NetworkServer::stop()
{
    if (not _running) return;
    _running = false;
    ServerSocket::network_gone();

    boost::system::error_code ec;
    _acceptor.cancel(ec);
    _io_service.stop();

    // Booost::asio hangs, despite the above.  Brute-force it to
    // get it's head out of it's butt, and do the right thing.
    pthread_cancel(_listener_thread->native_handle());

    _listener_thread->join();
    delete _listener_thread;
    _listener_thread = nullptr;
}

void NetworkServer::listen(void)
{
    prctl(PR_SET_NAME, "cogserv:listen", 0, 0, 0);
    printf("%s listening on port %d\n", _name.c_str(), _port);
    while (_running)
    {
        // The call to _acceptor.accept() will block this thread until
        // a network connection is made. Thus, we defer the creation
        // of the connection handler thread until after accept()
        // returns.  However, the boost design violates RAII principles,
        // so instead, what we do is to hand off the socket created here,
        // to the ServerSocket class, which will delete it, when the
        // connection socket closes (just before the connection handler
        // thread exits).  That is why there is no delete of the *ss
        // below, and that is why there is the weird self-delete at the
        // end of ServerSocket::handle_connection().
        boost::asio::ip::tcp::socket* sock = new boost::asio::ip::tcp::socket(_io_service);

        _acceptor.accept(*sock);

        // Exit, if cogserver is being shut down.
        if (not _running) break;

        _nconnections++;
        _last_connect = time(nullptr);

        boost::asio::ip::tcp::no_delay ndly(true);
        sock->set_option(ndly);

        int fd = sock->native_handle();
        // We are going to be sending oceans of tiny packets,
        // and we want the fastest-possible responses.
        int flags = 1;
        setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flags, sizeof(flags));
        flags = 1;
        setsockopt(fd, IPPROTO_TCP, TCP_QUICKACK, &flags, sizeof(flags));

        // The total number of concurrently open sockets is managed by
        // keeping a count in ConsoleSocket, and blocking when there are
        // too many.
        ServerSocket* ss = _getServer();
        ss->set_connection(sock);
        std::thread(&ServerSocket::handle_connection, ss).detach();
    }
}

void NetworkServer::run(ServerSocket* (*handler)(void))
{
    if (_running) return;
    _running = true;
    _getServer = handler;

    try {
        _io_service.run();
    } catch (boost::system::system_error& e) {
        logger().error("Error in boost::asio io_service::run() => %s", e.what());
    }

    _listener_thread = new std::thread(&NetworkServer::listen, this);
}

// ==================================================================

std::string NetworkServer::display_stats(int nlines)
{
    struct tm tm;
    char sbuf[40];
    gmtime_r(&_start_time, &tm);
    strftime(sbuf, 40, "%d %b %H:%M:%S %Y", &tm);

    char nbuf[40];
    time_t now = time(nullptr);
    gmtime_r(&now, &tm);
    strftime(nbuf, 40, "%d %b %H:%M:%S %Y", &tm);

    // Current max_open_sockets is 60 which requires a terminal
    // size of 66x80 to display correctly. So reserve a reasonble
    // string size.
    std::string rc;
    rc.reserve(4000);

    rc = "----- OpenCog CogServer top threads: type help or ^C to exit\n";
    rc += nbuf;
    rc += " UTC ---- up-since: ";
    rc += sbuf;
    rc += "\n";

    gmtime_r(&_last_connect, &tm);
    strftime(nbuf, 40, "%d %b %H:%M:%S", &tm);

    char buff[180];
    snprintf(buff, sizeof(buff),
        "status: %s  last: %s  tot-cnct: %4zd  port: %d\n",
        _running?"running":"halted", nbuf, _nconnections, _port);

    rc += buff;

    // Count open file descs
    int nfd = 0;
    for (int j=0; j<4096; j++) {
       int fd = dup(j);
       if (fd < 0) continue;
       close(fd);
       nfd++;
    }

    snprintf(buff, sizeof(buff),
        "max-open-socks: %d   cur-open-socks: %d   num-open-fds: %d  stalls: %zd\n",
        ConsoleSocket::get_max_open_sockets(),
        ConsoleSocket::get_num_open_sockets(),
        nfd,
        ConsoleSocket::get_num_open_stalls());
    rc += buff;

    clock_t clk = clock();
    int sec = clk / CLOCKS_PER_SEC;
    clock_t rem = clk - sec * CLOCKS_PER_SEC;
    int msec = (1000 * rem) / CLOCKS_PER_SEC;

    struct rusage rus;
    getrusage(RUSAGE_SELF, &rus);

    snprintf(buff, sizeof(buff),
        "cpu: %d.%03d secs  user: %ld.%03ld  sys: %ld.%03ld     tot-lines: %lu\n",
        sec, msec,
        rus.ru_utime.tv_sec, rus.ru_utime.tv_usec / 1000,
        rus.ru_stime.tv_sec, rus.ru_stime.tv_usec / 1000,
        ServerSocket::total_line_count.load());
    rc += buff;

    snprintf(buff, sizeof(buff),
        "maxrss: %ld KB  majflt: %ld  inblk: %ld  outblk: %ld\n",
        rus.ru_maxrss, rus.ru_majflt, rus.ru_inblock, rus.ru_oublock);
    rc += buff;

    // The above chews up 8 lines of display. Byobu/tmux needs a line.
    // Blank line for accepting commmands. So subtract 10.
    rc += "\n";
    rc += ServerSocket::display_stats(nlines - 10);

    return rc;
}

// ==================================================================
