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

#include <asio/ip/tcp.hpp>
#include <opencog/util/Logger.h>
#include <opencog/network/ServerSocket.h>
#include <opencog/network/ConsoleSocket.h>

#include "NetworkServer.h"

using namespace opencog;

NetworkServer::NetworkServer(unsigned short port, const char* name, SocketManager* mgr) :
    _name(name),
    _port(port),
    _running(false),
    _acceptor(_io_service),
    _socket_manager(mgr)
{
    logger().debug("[NetworkServer] constructor for %s at %d", name, port);

    // Try IPv6 dual-stack mode first (accepts both IPv6 and IPv4)
    bool ipv6_success = false;
    try {
        _acceptor.open(asio::ip::tcp::v6());
        _acceptor.set_option(asio::socket_base::reuse_address(true));
        _acceptor.set_option(asio::ip::v6_only(false));
        _acceptor.bind(asio::ip::tcp::endpoint(asio::ip::tcp::v6(), port));
        _acceptor.listen();
        logger().info("[NetworkServer] dual-stack IPv4/IPv6 mode enabled");
        ipv6_success = true;
    }
    catch (const std::system_error& e) {
        logger().info("[NetworkServer] IPv6 not available (%s), falling back to IPv4-only mode",
                      e.what());
    }

    // Fall back to IPv4-only if IPv6 failed
    if (!ipv6_success) {
        try {
            _acceptor.open(asio::ip::tcp::v4());
            _acceptor.set_option(asio::socket_base::reuse_address(true));
            _acceptor.bind(asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port));
            _acceptor.listen();
            logger().info("[NetworkServer] IPv4-only mode enabled");
        }
        catch (const std::system_error& e) {
            logger().error("[NetworkServer] Failed to bind to port %d: %s", port, e.what());
            throw;
        }
    }

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
    _socket_manager->network_gone();

    std::error_code ec;
    _acceptor.cancel(ec);
    _io_service.stop();

    // asio hangs, despite the above.  Brute-force it to
    // get it's head out of it's butt, and do the right thing.
    pthread_cancel(_listener_thread->native_handle());

    _listener_thread->join();
    delete _listener_thread;
    _listener_thread = nullptr;

    // Join all connection handler threads to ensure complete shutdown.
    // This guarantees all TCP/IP packets have been processed and all
    // handler threads have finished before serverLoop() returns.
    logger().debug("[NetworkServer] Joining %zu handler threads",
                   _handler_threads.size());

    std::list<std::thread*> threads_to_join;
    {
        std::lock_guard<std::mutex> lock(_handler_threads_mtx);
        threads_to_join.swap(_handler_threads);
    }

    for (std::thread* thr : threads_to_join)
    {
        thr->join();
        delete thr;
    }

    logger().debug("[NetworkServer] All handler threads joined");
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
        // returns.  However, the asio design violates RAII principles,
        // so instead, what we do is to hand off the socket created here,
        // to the ServerSocket class, which will delete it, when the
        // connection socket closes (just before the connection handler
        // thread exits).  That is why there is no delete of the *ss
        // below, and that is why there is the weird self-delete at the
        // end of ServerSocket::handle_connection().
        asio::ip::tcp::socket* sock = new asio::ip::tcp::socket(_io_service);

        _acceptor.accept(*sock);

        // Exit, if cogserver is being shut down.
        if (not _running) break;

        _nconnections++;
        _last_connect = time(nullptr);

        asio::ip::tcp::no_delay ndly(true);
        sock->set_option(ndly);

        int fd = sock->native_handle();
        // We are going to be sending oceans of tiny packets,
        // and we want the fastest-possible responses.
        int flags = 1;
        setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flags, sizeof(flags));
        flags = 1;
        setsockopt(fd, IPPROTO_TCP, TCP_QUICKACK, &flags, sizeof(flags));

        // The total number of concurrently open sockets is managed by
        // the SocketManager, which blocks when there are too many.
        ServerSocket* ss = _getServer(_socket_manager);
        ss->set_connection(sock);

        // Create handler thread and track it for proper cleanup.
        std::thread* handler_thread = new std::thread(&ServerSocket::handle_connection, ss);
        {
            std::lock_guard<std::mutex> lock(_handler_threads_mtx);
            _handler_threads.push_back(handler_thread);
        }
    }
}

void NetworkServer::run(ServerSocket* (*handler)(SocketManager*))
{
    if (_running) return;
    _running = true;
    _getServer = handler;

    try {
        _io_service.run();
    } catch (const std::system_error& e) {
        logger().error("Error in asio io_service::run() => %s", e.what());
    }

    _listener_thread = new std::thread(&NetworkServer::listen, this);
}

// ==================================================================
