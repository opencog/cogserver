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
#include <opencog/network/ConsoleSocket.h>

#include "NetworkServer.h"

using namespace opencog;

NetworkServer::NetworkServer(unsigned short port) :
    _running(false),
    _port(port),
    _acceptor(_io_service,
        boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
{
    logger().debug("[NetworkServer] constructor");
    _start_time = time(nullptr);
}

NetworkServer::~NetworkServer()
{
    logger().debug("[NetworkServer] enter destructor");

    stop();

    logger().debug("[NetworkServer] all threads joined, exit destructor");
}

void NetworkServer::stop()
{
    if (not _running) return;
    _running = false;

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
    printf("Listening on port %d\n", _port);
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
        ConsoleSocket* ss = _getConsole();
        ss->set_connection(sock);
        std::thread(&ConsoleSocket::handle_connection, ss).detach();
    }
}

void NetworkServer::run(ConsoleSocket* (*handler)(void))
{
    if (_running) return;
    _running = true;
    _getConsole = handler;

    try {
        _io_service.run();
    } catch (boost::system::system_error& e) {
        logger().error("Error in boost::asio io_service::run() => %s", e.what());
    }

    _listener_thread = new std::thread(&NetworkServer::listen, this);
}

// ==================================================================

std::string NetworkServer::display_stats(void)
{
    struct tm tm;
    char sbuf[40];
    gmtime_r(&_start_time, &tm);
    strftime(sbuf, 40, "%d %b %H:%M:%S %Y", &tm);

    char nbuf[40];
    time_t now = time(nullptr);
    gmtime_r(&now, &tm);
    strftime(nbuf, 40, "%d %b %H:%M:%S %Y", &tm);

    std::string rc = "-----\n";
    rc += nbuf;
    rc += " ---- Up since: ";
    rc += sbuf;
    rc += "\n";

    char buff[80];
    snprintf(buff, 80, "Status: %s  Port: %d\n",
        _running?"running":"halted", _port);

    rc += buff;

    // count open file descs
    int nfd = 0;
    for (int j=0; j<4096; j++) {
       int fd = dup(j);
       if (fd < 0) continue;
       close(fd);
       nfd++;
    }

    snprintf(buff, 80,
        "max-open-socks: %d   cur-open-socks: %d   num-open-fds: %d\n",
        ConsoleSocket::get_max_open_sockets(),
        ConsoleSocket::get_num_open_sockets(),
        nfd);
    rc += buff;

    clock_t clk = clock();
    int sec = clk / CLOCKS_PER_SEC;
    clock_t rem = clk - sec * CLOCKS_PER_SEC;
    int msec = (1000 * rem) / CLOCKS_PER_SEC;

    struct rusage rus;
    getrusage(RUSAGE_SELF, &rus);

    snprintf(buff, 80,
        "cpu: %d.%03d secs  user: %ld.%03ld  sys: %ld.%03ld\n",
        sec, msec,
        rus.ru_utime.tv_sec, rus.ru_utime.tv_usec / 1000,
        rus.ru_stime.tv_sec, rus.ru_stime.tv_usec / 1000);
    rc += buff;

    snprintf(buff, 80,
        "maxrss: %ld KB  majflt: %ld  inblk: %ld  outblk: %ld\n",
        rus.ru_maxrss, rus.ru_majflt, rus.ru_inblock, rus.ru_oublock);
    rc += buff;

    rc += "\n";
    rc += ServerSocket::display_stats();
    rc += "\n";

    return rc;
}

// ==================================================================
